/* Simple NTP client */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>
#ifdef PRECISION_SIOCGSTAMP
#include <sys/ioctl.h>
#endif
#ifdef __linux__
#include <sys/timex.h>
#endif

#include "logit.h"
#include "sntp_net.h"
#include "phaselock.h"
#include "sntpd.h"

int dry = 0;                        /* Dry run, no time corrections */
static volatile sig_atomic_t sighup = 0;
static volatile sig_atomic_t sigterm = 0;
struct ntp_peers peer;
double root_delay;
double root_dispersion;


/* OS dependent routine to get the current value of clock frequency */
static int get_current_freq(void)
{
#ifdef __linux__
    struct timex txc;

    txc.modes = 0;
    if (adjtimex(&txc) < 0)
    {
        ERR(errno, "Failed adjtimex(GET)");
        return 0;
    }
    return txc.freq;
#else
    return 0;
#endif
}

/* OS dependent routine to set a new value of clock frequency */
static int set_freq(int new_freq)
{
#ifdef __linux__
    struct timex txc;

    txc.modes = ADJ_FREQUENCY;
    txc.freq = new_freq;
    if (adjtimex(&txc) < 0)
    {
        ERR(errno, "Failed adjtimex(SET)");
        return 0;
    }
    return txc.freq;
#else
    return 0;
#endif
}

static void print_time(struct timespec *cur)
{
    char buf[64];
    struct tm *timeinfo;

    timeinfo = localtime((time_t *)&cur->tv_sec);
    strftime(buf, sizeof(buf), "%F %T %z", timeinfo);

    INFO("set time[%s], nsec[%.9lu]", buf, cur->tv_nsec);
}

static void set_time(struct ntptime *new)
{
    struct timespec tv_set;

    /* it would be even better to subtract half the slop */
    tv_set.tv_sec = new->coarse - JAN_1970;
    /* divide xmttime.fine by 4294.967296 */
    tv_set.tv_nsec = USEC(new->fine) * 1000;
    if (clock_settime(CLOCK_REALTIME, &tv_set) < 0)
    {
        ERR(errno, "Failed clock_settime()");
        return;
    }

    print_time(&tv_set);
    //DBG("Set time to %lu.%.9lu", tv_set.tv_sec, tv_set.tv_nsec);
}

void ntpc_gettime(struct ntptime *nt)
{
    struct timespec now;

    clock_gettime(CLOCK_REALTIME, &now);
    nt->coarse = now.tv_sec + JAN_1970;
    nt->fine = NTPFRAC(now.tv_nsec / 1000);
}

static int send_packet(int usd, struct ntptime *time_sent)
{
    uint32_t data[12];

#define LI 0
#define VN 3
#define MODE 3
#define STRATUM 0
#define POLL 4
#define PREC -6

#ifdef ENABLE_DEBUG
        DBG("Sending packet ...");
#endif
    if (sizeof(data) != 48)
    {
        ERR(0, "Packet size error");
        return -1;
    }

    memset(data, 0, sizeof data);
    data[0] = htonl((LI << 30) | (VN << 27) | (MODE << 24) | (STRATUM << 16) | (POLL << 8) | (PREC & 0xff));
    data[1] = htonl(1 << 16); /* Root Delay (seconds) */
    data[2] = htonl(1 << 16); /* Root Dispersion (seconds) */
    ntpc_gettime(time_sent);

    data[10] = htonl(time_sent->coarse); /* Transmit Timestamp coarse */
    data[11] = htonl(time_sent->fine);   /* Transmit Timestamp fine   */

    return send(usd, data, 48, 0);
}

void get_packet_timestamp(int usd, struct ntptime *udp_arrival_ntp)
{
#ifdef PRECISION_SIOCGSTAMP
    struct timeval udp_arrival;

    if (ioctl(usd, SIOCGSTAMP, &udp_arrival) < 0)
    {
        ERR(errno, "Failed ioctl(SIOCGSTAMP)");
        ntpc_gettime(udp_arrival_ntp);
    }
    else
    {
        udp_arrival_ntp->coarse = udp_arrival.tv_sec + JAN_1970;
        udp_arrival_ntp->fine = NTPFRAC(udp_arrival.tv_usec);
    }
#else
    (void)usd; /* not used */
    ntpc_gettime(udp_arrival_ntp);
#endif
}

static double ntpdiff(struct ntptime *start, struct ntptime *stop)
{
    int a;
    unsigned int b;

    a = stop->coarse - start->coarse;
    if (stop->fine >= start->fine)
    {
        b = stop->fine - start->fine;
    }
    else
    {
        b = start->fine - stop->fine;
        b = ~b;
        a -= 1;
    }

    return a * 1.e6 + b * (1.e6 / 4294967296.0);
}

/* Does more than print, so this name is bogus.
 * It also makes time adjustments, both sudden (-s)
 * and phase-locking (-l).
 * sets *error to the number of microseconds uncertainty in answer
 * returns 0 normally, 1 if the message fails sanity checks
 */
static int rfc1305print(uint32_t *data, struct ntptime *arrival, struct ntp_control *ntpc, int *error)
{
    static int first = 1;

    /* straight out of RFC-1305 Appendix A */
    int li, vn, mode, stratum, prec;
    int delay, disp;

#ifdef ENABLE_DEBUG
    int poll, refid;
    struct ntptime reftime;
#endif
    struct ntptimes pkt_root_delay, pkt_root_dispersion;
    struct ntptime orgtime, rectime, xmttime;
    double el_time, st_time, skew1, skew2, dtemp;
    int freq;
    const char *drop_reason = NULL;

#define Data(i) ntohl(((uint32_t *)data)[i])
    li = Data(0) >> 30 & 0x03;
    vn = Data(0) >> 27 & 0x07;
    mode = Data(0) >> 24 & 0x07;
    stratum = Data(0) >> 16 & 0xff;
#ifdef ENABLE_DEBUG
    poll = Data(0) >> 8 & 0xff;
#endif
    prec = Data(0) & 0xff;
    if (prec & 0x80)
        prec |= 0xffffff00;
    delay = Data(1);
    disp = Data(2);

#ifdef ENABLE_DEBUG
    refid = Data(3);
    reftime.coarse = Data(4);
    reftime.fine = Data(5);
#endif

    orgtime.coarse = Data(6);
    orgtime.fine = Data(7);
    rectime.coarse = Data(8);
    rectime.fine = Data(9);
    xmttime.coarse = Data(10);
    xmttime.fine = Data(11);
#undef Data

#ifdef ENABLE_DEBUG
    DBG("LI=%d  VN=%d  Mode=%d  Stratum=%d  Poll=%d  Precision=%d", li, vn, mode, stratum, poll, prec);
    DBG("Delay=%.1f  Dispersion=%.1f  Refid=%u.%u.%u.%u", sec2u(delay), sec2u(disp),
        refid >> 24 & 0xff, refid >> 16 & 0xff, refid >> 8 & 0xff, refid & 0xff);
    DBG("Reference %u.%.6u", reftime.coarse, USEC(reftime.fine));
    DBG("(sent)    %u.%.6u", ntpc->time_of_send.coarse, USEC(ntpc->time_of_send.fine));
    DBG("Originate %u.%.6u", orgtime.coarse, USEC(orgtime.fine));   /* T1 */
    DBG("Receive   %u.%.6u", rectime.coarse, USEC(rectime.fine));   /* T2 */
    DBG("Transmit  %u.%.6u", xmttime.coarse, USEC(xmttime.fine));   /* T3 */
    DBG("Our recv  %u.%.6u", arrival->coarse, USEC(arrival->fine)); /* T4 */
#endif

    el_time = ntpdiff(&orgtime, arrival);  /* elapsed: (T4 - T1)*/
    st_time = ntpdiff(&rectime, &xmttime); /* stall: (T3 - T2) */
    skew1 = ntpdiff(&orgtime, &rectime);
    skew2 = ntpdiff(&xmttime, arrival);
    freq = get_current_freq();
    *error = el_time - st_time;

#ifdef ENABLE_DEBUG
    DBG("Total elapsed: %9.2f", el_time);
    DBG("Server stall:  %9.2f", st_time);
    DBG("Slop:          %9.2f", el_time - st_time);
    DBG("Skew:          %9.2f", (skew1 - skew2) / 2);
    DBG("Frequency:     %9d", freq);
#endif

    /* error checking, see RFC-4330 section 5 */
#define FAIL(x)            \
    do                     \
    {                      \
        drop_reason = (x); \
        goto fail;         \
    } while (0)
    if (ntpc->cross_check)
    {
        if (li == 3)
            FAIL("LI==3"); /* unsynchronized */
        if (vn < 3)
            FAIL("VN<3"); /* RFC-4330 documents SNTP v4, but we interoperate with NTP v3 */
        if (mode != 4)
            FAIL("MODE!=3");
        if (orgtime.coarse != ntpc->time_of_send.coarse || orgtime.fine != ntpc->time_of_send.fine)
            FAIL("ORG!=sent");
        if (xmttime.coarse == 0 && xmttime.fine == 0)
            FAIL("XMT==0");
        if (delay > 65536 || delay < -65536)
            FAIL("abs(DELAY)>65536");
        if (disp > 65536 || disp < -65536)
            FAIL("abs(DISP)>65536");
        if (stratum == 0)
            FAIL("STRATUM==0"); /* kiss o' death */
#undef FAIL
    }

    if (!dry && ntpc->set_clock)
    {
        /* CAP_SYS_TIME or root required, or sntpd will exit here! */
        set_time(&xmttime);
        DBG("Time synchronized to server %s, stratum %d", ntpc->server, stratum);
    }

    /* Update last time set ... */
    pkt_root_delay.coarse = ((uint32_t)delay >> 16) & 0xFFFF;
    pkt_root_delay.fine = ((uint32_t)delay >> 0) & 0xFFFF;
    pkt_root_dispersion.coarse = ((uint32_t)disp >> 16) & 0xFFFF;
    pkt_root_dispersion.fine = ((uint32_t)disp >> 0) & 0xFFFF;

    peer.last_update_ts = xmttime;
    peer.last_rootdelay = wire2d32(&pkt_root_delay);
    peer.last_rootdisp = wire2d32(&pkt_root_dispersion);

    /* delay = (T4 - T1) - (T3 - T2) */
    peer.last_delay = (wire2d64(arrival) - wire2d64(&orgtime)) - (wire2d64(&xmttime) - wire2d64(&rectime));
    if (peer.last_delay < G_precision_sec)
        peer.last_delay = G_precision_sec;

    root_delay = peer.last_rootdelay + peer.last_delay;
    dtemp = G_precision_sec + MIN_DISP; /* XXX: Fixme, see BusyBox ntpd.c */
    root_dispersion = peer.last_rootdisp + dtemp;
    //LOG("Calculated root_delay %f, root_dispersion %f", root_delay, root_dispersion);

    /*
     * Not the ideal order for printing, but we want to be sure
     * to do all the time-sensitive thinking (and time setting)
     * before we start the output, especially fflush() (which
     * could be slow).  Of course, if debug is turned on, speed
     * has gone down the drain anyway.
     */
    if (ntpc->live)
    {
        int new_freq;

        new_freq = contemplate_data(arrival->coarse,
                                    (skew1 - skew2) / 2,
                                    el_time + sec2u(disp),
                                    freq);

        if (!dry && new_freq != freq)
            set_freq(new_freq);
    }

    if (!ntpc->set_clock)
    {
        /*
         * Display by default for ntpclient users, sntpd must run with -l info:
         * Day   Second      Elapsed    Stall      Skew  Dispersion  Freq
         * 43896 75886.786    6653.0      2.4     798.9  14663.7     92907
         */
        if (first)
        {
            INFO("Day   Second      Elapsed    Stall      Skew  Dispersion   Freq");
            first = 0;
        }
        INFO("%d %.5d.%.3d  %8.1f %8.1f  %8.1f %8.1f %9d",
             arrival->coarse / 86400, arrival->coarse % 86400,
             arrival->fine / 4294967, el_time, st_time,
             (skew1 - skew2) / 2, sec2u(disp), freq);
    }

    return 0;

fail:
    ERR(0, "%d %.5d.%.3d rejected packet: %s",
        arrival->coarse / 86400, arrival->coarse % 86400,
        arrival->fine / 4294967, drop_reason);

    return 1;
}

/*
 * Signal handler.  Take note of the fact that the signal arrived
 * so that the main loop can take care of it.
 */
static void handler(int sig)
{
    switch (sig)
    {
    case SIGHUP:
    case SIGALRM:
        /* Trigger NTP sync */
        sighup = 1;
        break;

    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
    case SIGUSR1:
    case SIGUSR2:
        sigterm = 1;
        break;
    }
}

static void setup_signals(void)
{
    struct sigaction sa;

    sa.sa_handler = handler;
    sa.sa_flags = 0; /* Interrupt system calls */
    sigemptyset(&sa.sa_mask);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
}

static void loop(struct ntp_control *ntpc)
{
    fd_set fds;
    struct sockaddr_storage sa_xmit;
    int i, pack_len, probes_sent, error;
    socklen_t sa_xmit_len;
    struct timeval to;
    struct ntptime udp_arrival_ntp;
    static uint32_t incoming_word[325];
    int usd = -1;
    int sd = -1;

#define incoming ((char *)incoming_word)
#define sizeof_incoming (sizeof incoming_word)

    if (ntpc->server_port)
        sd = server_init(ntpc->server_port);

#ifdef ENABLE_DEBUG
        DBG("Listening...");
#endif
    probes_sent = 0;
    sa_xmit_len = sizeof(sa_xmit);
    to.tv_sec = 0;
    to.tv_usec = 0;

    while (1)
    {
        if (sigterm)
        {
            ntpc->live = 0;
            break;
        }

        if (sighup || usd == -1)
        {
            int init;

            sighup = 0;
            to.tv_sec = 0;
            to.tv_usec = 0;

            if (usd == -1)
                init = 1;
            else
                close(usd);

            usd = setup_socket(ntpc->server, ntpc->udp_port, ntpc->local_udp_port);
            if (usd == -1)
            {
                /* Wait here a while, networking is probably not up yet. */
                if (errno == ENETDOWN)
                {
                    sleep(1);
                    continue;
                }
                ERR(errno, init ? "Failed creating UDP socket()"
                                : "Failed reopening NTP socket");
                goto done;
            }

            if (!init)
                DBG("Got SIGHUP, triggering resync with NTP server.");
            init = 0;
        }

        FD_ZERO(&fds);
        FD_SET(usd, &fds);
        if (sd > -1)
            FD_SET(sd, &fds);

        i = select(usd + 1, &fds, NULL, NULL, &to); /* Wait on read or error */
        if (i <= 0)
        {
            if (i < 0)
            {
                if (errno != EINTR)
                    ERR(errno, "Failed select()");
                continue;
            }

            if (to.tv_sec == 0)
            {
                if (probes_sent >= ntpc->probe_count && ntpc->probe_count != 0)
                    break;

                if (send_packet(usd, &ntpc->time_of_send) == -1)
                {
                    ERR(errno, "Failed sending probe");
                    to.tv_sec = MIN_INTERVAL;
                    to.tv_usec = 0;
                }
                else
                {
                    ++probes_sent;
                    to.tv_sec = ntpc->cycle_time;
                    to.tv_usec = 0;
                }
            }
            continue;
        }

        if (sd > -1 && FD_ISSET(sd, &fds))
        {
            server_recv(sd);
            continue;
        }

        error = ntpc->goodness;
        pack_len = recvfrom(usd, incoming, sizeof_incoming, 0, (struct sockaddr *)&sa_xmit, &sa_xmit_len);
        if (pack_len < 0)
        {
            ERR(errno, "Failed recvfrom()");
        }
        else if (pack_len > 0 && (unsigned)pack_len < sizeof_incoming)
        {
            get_packet_timestamp(usd, &udp_arrival_ntp);
            if (check_source(pack_len, &sa_xmit, ntpc->udp_port))
                continue;
            if (rfc1305print(incoming_word, &udp_arrival_ntp, ntpc, &error) != 0)
                continue;
        }
        else
        {
            ERR(0, "Ooops.  pack_len=%d", pack_len);
        }

        if ((error < ntpc->goodness && ntpc->goodness != 0) ||
            (probes_sent >= ntpc->probe_count && ntpc->probe_count != 0))
        {
            ntpc->set_clock = 0;
            if (!ntpc->live)
                break;
        }
    }
#undef incoming
#undef sizeof_incoming
done:
    if (usd != -1)
        close(usd);
    if (sd != -1)
        close(sd);
}

static void run(struct ntp_control *ntpc, int log_level)
{
    /*
     * Force output to syslog, we have no other way of
     * communicating with the user after being daemonized
     */
    if (ntpc->daemonize)
    {
        ntpc->syslog = 1;
    }
    log_init(MOD_NAME, log_level, ntpc->syslog);

    if (ntpc->initial_freq)
    {
        DBG("Initial frequency %d", ntpc->initial_freq);
        set_freq(ntpc->initial_freq);
    }

    if (ntpc->set_clock && !ntpc->live && !ntpc->goodness && !ntpc->probe_count)
        ntpc->probe_count = 1;

    /* If user gives a probe count, then assume non-live run */
    if (ntpc->probe_count > 0)
        ntpc->live = 0;

    /* respect only applicable MUST of RFC-4330 */
    if (ntpc->probe_count != 1 && ntpc->cycle_time < MIN_INTERVAL)
        ntpc->cycle_time = MIN_INTERVAL;

#ifdef ENABLE_DEBUG
    DBG("Configuration:");
    DBG("  probe_count %d", ntpc->probe_count);
    DBG("  Dry run     %d", dry);
    DBG("  goodness    %d", ntpc->goodness);
    DBG("  hostname    %s", ntpc->server);
    DBG("  interval    %d", ntpc->cycle_time);
    DBG("  live        %d", ntpc->live);
    DBG("  local_port  %d", ntpc->local_udp_port);
    DBG("  set_clock   %d", ntpc->set_clock);
    DBG("  cross_check %d", ntpc->cross_check);
#endif

    /* Startup sequence */
    if (ntpc->daemonize)
    {
        if (-1 == daemon(0, 0))
        {
            ERR(errno, "Failed daemonizing, aborting");
            exit(1);
        }
    }

    if (!ntpc->usermode)
        LOG("Starting " MOD_NAME " v"  MOD_VERSION);
    setup_signals();

    INFO("Using time sync server: %s", ntpc->server);

    loop(ntpc);

    if (!ntpc->usermode)
        LOG("Stopping "  MOD_NAME " v"  MOD_VERSION);

    log_exit();
}

#if 0
static int ntpclient_usage(int code)
{
    FILE *fp = stdout;

    if (code)
        fp = stderr;

    fprintf(fp,
            "Usage:\n"
            "  ntpclient [-c count] [-d] [-f frequency] [-g goodness] -h hostname\n"
            "            [-i interval] [-l] [-p port] [-q min_delay] [-r] [-s] [-t]\n"
            "\n"
            "Options:\n"
            "  -c count      stop after count time measurements (default 0 means go forever)\n"
            "  -d            Dry run, connect to server, do calculations, no time correction\n"
            "  -f frequency  Initialize frequency offset.  Linux only, requires CAP_SYS_TIME\n"
            "  -g goodness   causes ntpclient to stop after getting a result more accurate\n"
            "                than goodness (microseconds, default 0 means go forever)\n"
            "  -h hostname   (mandatory) NTP server, against which to measure system time\n"
            "  -i interval   check time every interval seconds (default 600)\n"
            "  -l            attempt to lock local clock to server using adjtimex(2)\n"
            "  -p port       local NTP client UDP port (default 0 means \"any available\")\n"
            "  -q min_delay  minimum packet delay for transaction (default 800 microseconds)\n"
            "  -s            simple clock set (implies -c 1)\n"
            "  -t            trust network and server, no RFC-4330 recommended cross-checks\n"
            "\n"

    return code;
}
#endif

int ntpclient(char *hostname, uint16_t port)
{
    struct ntp_control ntpc;

    memset(&ntpc, 0, sizeof(ntpc));
    ntpc.set_clock      = 1;
    ntpc.cycle_time     = 600;
    ntpc.probe_count    = 0;  
    ntpc.goodness       = 0;
    ntpc.live           = 0;
    ntpc.usermode       = 1;
    ntpc.cross_check    = 1;
    ntpc.initial_freq   = 0;
    ntpc.udp_port       = port ? port : NTP_PORT;       /* port不能是0 */
    ntpc.local_udp_port = 0;
    ntpc.daemonize      = 0;
    ntpc.syslog         = 0;
    ntpc.server         = hostname;
    if (!ntpc.server)
    {
        ERR(errno, "hostname invalid\n");
        return 0;
    }

    run(&ntpc, 0 ? LOG_DEBUG : LOG_INFO);

    return 0;
}

size_t compose_reply(struct ntp *ntp, size_t len)
{
    char id[] = "LOCL";

    /* li:0, version:3, mode:4 (server) */
    ntp->flags = 0 << 6 | 3 << 3 | 4 << 0;

    ntp->stratum = 1;
    ntp->interval = 4; /* 16 s */
    ntp->precision = G_precision_exp;

    /* root delay and rooty dispersion */
    ntp->delay = u2sec(root_delay);
    ntp->dispersion = u2sec(root_dispersion);

    memcpy(ntp->identifier, id, sizeof(ntp->identifier));

    return sizeof(*ntp);
}

