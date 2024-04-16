/* NTP client */
#ifndef _SNTPD_H_
#define _SNTPD_H_

#include <limits.h>         /* UINT_MAX */
#include <unistd.h>
#include <arpa/inet.h>      /* htons */


#define MOD_NAME        "sntpd"
#define MOD_VERSION     "1.0"
#define MOD_STRING      "sntpd 1.0"

/* 
 * Default to the RFC-4330 specified value
 * A client MUST NOT under any conditions use a poll interval less
 * than 15 seconds
 */
#ifndef MIN_INTERVAL
#define MIN_INTERVAL 15
#endif

#ifndef MIN_DISP
#define MIN_DISP 0.01
#endif

#define JAN_1970 0x83aa7e80 /* 2208988800 1970 - 1900 in seconds */
#define NTP_PORT (123)

/* How to multiply by 4294.967296 quickly (and not quite exactly)
 * without using floating point or greater than 32-bit integers.
 * If you want to fix the last 12 microseconds of error, add in
 * (2911*(x))>>28)
 */
#define NTPFRAC(x) (4294 * (x) + ((1981 * (x)) >> 11))

/* The reverse of the above, needed if we want to set our microsecond
 * clock (via clock_settime) based on the incoming time in NTP format.
 * Basically exact.
 */
#define USEC(x) (((x) >> 12) - 759 * ((((x) >> 10) + 32768) >> 16))

/* Converts NTP delay and dispersion, apparently in seconds scaled
 * by 65536, to microseconds.  RFC-1305 states this time is in seconds,
 * doesn't mention the scaling.
 * Should somehow be the same as 1000000 * x / 65536
 */
#define sec2u(x) ((x) * 15.2587890625)

/* precision is defined as the larger of the resolution and time to
 * read the clock, in log2 units.  For instance, the precision of a
 * mains-frequency clock incrementing at 60 Hz is 16 ms, even when the
 * system clock hardware representation is to the nanosecond.
 *
 * Delays, jitters of various kinds are clamped down to precision.
 *
 * If precision_sec is too large, discipline_jitter gets clamped to it
 * and if offset is smaller than discipline_jitter * POLLADJ_GATE, poll
 * interval grows even though we really can benefit from staying at
 * smaller one, collecting non-lagged datapoits and correcting offset.
 * (Lagged datapoits exist when poll_exp is large but we still have
 * systematic offset error - the time distance between datapoints
 * is significant and older datapoints have smaller offsets.
 * This makes our offset estimation a bit smaller than reality)
 * Due to this effect, setting G_precision_sec close to
 * STEP_THRESHOLD isn't such a good idea - offsets may grow
 * too big and we will step. I observed it with -6.
 *
 * OTOH, setting precision_sec far too small would result in futile
 * attempts to synchronize to an unachievable precision.
 *
 * -6 is 1/64 sec, -7 is 1/128 sec and so on.
 * -8 is 1/256 ~= 0.003906 (worked well for me --vda)
 * -9 is 1/512 ~= 0.001953 (let's try this for some time)
 */
#define G_precision_exp -9
/*
 * G_precision_exp is used only for construction outgoing packets.
 * It's ok to set G_precision_sec to a slightly different value
 * (One which is "nicer looking" in logs).
 * Exact value would be (1.0 / (1 << (- G_precision_exp))):
 */
#define G_precision_sec 0.002

struct ntptime
{
    uint32_t coarse;
    uint32_t fine;
};

struct ntptimes
{
    uint16_t coarse;
    uint16_t fine;
};

struct ntp_control
{
    struct ntptime time_of_send;
    int usermode;                       /* 0: sntpd, 1: ntpclient */
    int daemonize;                      /* 以守护进程方式运行 */
    int syslog;                         /* 是否使用syslog */
    int live;                           /* 是否使用adjtimex同步时钟，不建议使用 */
    int set_clock;                      /* non-zero presumably needs CAP_SYS_TIME or root */
    int probe_count;                    /* default of 0 means loop forever */
    int cycle_time;
    int goodness;
    int cross_check;
    int initial_freq;                   /* initial freq value to use */

    uint16_t local_udp_port;            /* local bind port；0：系统会选择一个可用的端口号 */
    uint16_t udp_port;                  /* remote port on 'server' */
    uint16_t server_port;
    char *server;                       /* must be set in client mode */
    char serv_addr[4];
};

struct ntp
{
    uint8_t flags;                      /*  0: */
    uint8_t stratum;                    /*  1: */
    uint8_t interval;                   /*  2: */
    int8_t precision;                   /*  3: */

    struct ntptimes delay;              /*  4: */
    struct ntptimes dispersion;         /*  8: */

    char identifier[4];                 /* 12: */

    struct ntptime refclk_ts;           /* 16: */
    struct ntptime origin_ts;           /* 24: */
    struct ntptime recv_ts;             /* 32: */
    struct ntptime xmit_ts;             /* 40: */
};

struct ntp_peers
{
    struct ntptime last_update_ts;
    double last_rootdelay;
    double last_rootdisp;
    double last_delay;
};

void get_packet_timestamp(int usd, struct ntptime *nt);
void ntpc_gettime(struct ntptime *nt);

/* Workaround for missing SIOCGSTAMP after Linux 5.1 64-bit timestamp fixes. */
#ifndef SIOCGSTAMP
#ifdef SIOCGSTAMP_OLD
#define SIOCGSTAMP SIOCGSTAMP_OLD
#else
#define SIOCGSTAMP 0x8906
#endif
#endif

/* Misc. helpers */
static inline double wire2d64(struct ntptime *nt)
{
    return (double)nt->coarse + ((double)nt->fine / UINT_MAX);
}

static inline double wire2d32(struct ntptimes *nt)
{
    return (double)nt->coarse + ((double)nt->fine / USHRT_MAX);
}

static inline struct ntptimes u2sec(double x)
{
    struct ntptimes nts;

    nts.coarse = (uint16_t)x;
    nts.fine = (uint16_t)((x - nts.coarse) * USHRT_MAX);
    //DBG("double %f => 32-bit int %d.%d", x, nts.coarse, nts.fine);

    nts.coarse = htons(nts.coarse);
    nts.fine = htons(nts.fine);

    return nts;
}

#endif /* _SNTPD_H_ */

