
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "deftypes.h"
#include "usr_time.h"


#define VERSION_3       3
#define VERSION_4       4

#define MODE_CLIENT     3
#define MODE_SERVER     4

// NTP protocol Content
#define NTP_LI          0
#define NTP_VN          VERSION_3
#define NTP_MODE        MODE_CLIENT
#define NTP_STRATUM     0
#define NTP_POLL        4
#define NTP_PRECISION   -6

#define NTP_HLEN        48

#define NTP_PORT        123
#define NTP_SRV         "time.windows.com"


#define TIMEOUT         1

#define BUFSIZE         128

#define JAN_1970        0x83aa7e80

#define NTP_CONV_FRAC32(x) (uint64_t)((x) * ((uint64_t)1 << 32))
#define NTP_REVE_FRAC32(x) ((double)((double)(x) / ((uint64_t)1 << 32)))

#define NTP_CONV_FRAC16(x) (uint32_t)((x) * ((uint32_t)1 << 16))
#define NTP_REVE_FRAC16(x) ((double)((double)(x) / ((uint32_t)1 << 16)))

#define USEC2FRAC(x) ((uint32_t)NTP_CONV_FRAC32((x) / 1000000.0))
#define FRAC2USEC(x) ((uint32_t)NTP_REVE_FRAC32((x) * 1000000.0))

#define NTP_LFIXED2DOUBLE(x)                                         \
    ((double)(ntohl(((struct l_fixedpt *)(x))->intpart) - JAN_1970 + \
              FRAC2USEC(ntohl(((struct l_fixedpt *)(x))->fracpart)) / 1000000.0))

struct s_fixedpt
{
    uint16_t intpart;
    uint16_t fracpart;
};

struct l_fixedpt
{
    uint32_t intpart;
    uint32_t fracpart;
};

struct ntphdr
{
    unsigned int ntp_mode : 3;
    unsigned int ntp_vn : 3;
    unsigned int ntp_li : 2;

    uint8_t ntp_stratum;
    uint8_t ntp_poll;
    int8_t ntp_precision;
    struct s_fixedpt ntp_rtdelay;
    struct s_fixedpt ntp_rtdispersion;
    uint32_t ntp_refid;
    struct l_fixedpt ntp_refts;
    struct l_fixedpt ntp_orits;
    struct l_fixedpt ntp_recvts;
    struct l_fixedpt ntp_transts;
};

static in_addr_t inet_host(const char *host)
{
    in_addr_t saddr;
    struct hostent *hostent;

    if ((saddr = inet_addr(host)) == INADDR_NONE)
    {
        if ((hostent = gethostbyname(host)) == NULL)
        {
            return INADDR_NONE;
        }

        // memcpy(&saddr, hostent->h_addr_list[0], hostent->h_length);
        saddr = *((unsigned long *)hostent->h_addr_list[0]);
    }

    return saddr;
}

static int get_ntp_packet(void *buf, size_t *size)
{
    struct ntphdr *ntp;
    struct timeval tv;

    if (!size || *size < NTP_HLEN)
    {
        return -1;
    }

    memset(buf, 0, *size);

    ntp = (struct ntphdr *)buf;
    ntp->ntp_li = NTP_LI;
    ntp->ntp_vn = NTP_VN;
    ntp->ntp_mode = NTP_MODE;
    ntp->ntp_stratum = NTP_STRATUM;
    ntp->ntp_poll = NTP_POLL;
    ntp->ntp_precision = NTP_PRECISION;

    gettimeofday(&tv, NULL);
    ntp->ntp_transts.intpart = htonl(tv.tv_sec + JAN_1970);
    ntp->ntp_transts.fracpart = htonl(USEC2FRAC(tv.tv_usec));

    *size = NTP_HLEN;

    return 0;
}

//static 
void print_ntp(struct ntphdr *ntp)
{
    time_t time;

    printf("LI:\t%d \n", ntp->ntp_li);
    printf("VN:\t%d \n", ntp->ntp_vn);
    printf("Mode:\t%d \n", ntp->ntp_mode);
    printf("Stratum:\t%d \n", ntp->ntp_stratum);
    printf("Poll:\t%d \n", ntp->ntp_poll);
    printf("precision:\t%d \n", ntp->ntp_precision);

    printf("Route delay:\t %lf \n",
           ntohs(ntp->ntp_rtdelay.intpart) + NTP_REVE_FRAC16(ntohs(ntp->ntp_rtdelay.fracpart)));
    printf("Route Dispersion: %lf \n", ntohs(ntp->ntp_rtdispersion.intpart) +
                                           NTP_REVE_FRAC16(ntohs(ntp->ntp_rtdispersion.fracpart)));
    printf("Referencd ID:\t %u \n", ntohl(ntp->ntp_refid));

    time = ntohl(ntp->ntp_refts.intpart) - JAN_1970;
    printf("Reference:\t%u %d %s \n", ntohl(ntp->ntp_refts.intpart) - JAN_1970,
           FRAC2USEC(ntohl(ntp->ntp_refts.fracpart)), ctime(&time));

    time = ntohl(ntp->ntp_orits.intpart) - JAN_1970;
    printf("Originate:\t%u %d frac=%u (%s) \n", ntohl(ntp->ntp_orits.intpart) - JAN_1970,
           FRAC2USEC(ntohl(ntp->ntp_orits.fracpart)), ntohl(ntp->ntp_orits.fracpart), ctime(&time));

    time = ntohl(ntp->ntp_recvts.intpart) - JAN_1970;
    printf("Receive:\t%u %d (%s) \n", ntohl(ntp->ntp_recvts.intpart) - JAN_1970,
           FRAC2USEC(ntohl(ntp->ntp_recvts.fracpart)), ctime(&time));

    time = ntohl(ntp->ntp_transts.intpart) - JAN_1970;
    printf("Transmit:\t%u %d (%s) \n", ntohl(ntp->ntp_transts.intpart) - JAN_1970,
           FRAC2USEC(ntohl(ntp->ntp_transts.fracpart)), ctime(&time));
}

//static 
double get_rrt(const struct ntphdr *ntp, const struct timeval *recvtv)
{
    double t1, t2, t3, t4;

    t1 = NTP_LFIXED2DOUBLE(&ntp->ntp_orits);
    t2 = NTP_LFIXED2DOUBLE(&ntp->ntp_recvts);
    t3 = NTP_LFIXED2DOUBLE(&ntp->ntp_transts);
    t4 = recvtv->tv_sec + recvtv->tv_usec / 1000000.0;

    return (t4 - t1) - (t3 - t2);
}

static double get_offset(const struct ntphdr *ntp, const struct timeval *recvtv)
{
    double t1, t2, t3, t4;

    t1 = NTP_LFIXED2DOUBLE(&ntp->ntp_orits);
    t2 = NTP_LFIXED2DOUBLE(&ntp->ntp_recvts);
    t3 = NTP_LFIXED2DOUBLE(&ntp->ntp_transts);
    t4 = recvtv->tv_sec + recvtv->tv_usec / 1000000.0;

    return ((t2 - t1) + (t3 - t4)) / 2;
}

int ntp_sync_time(char *server, uint16_t port)
{
    struct sockaddr_in servaddr = {0};
    char *srv =  server ? server : NTP_SRV;
    uint16_t srv_port = port ? port : NTP_PORT;
    int sockfd, maxfd1, ret;
    char buf[BUFSIZE];    
    size_t nbytes;
    fd_set readfds;
    struct timeval timeout, recvtv, tv, rcvtimeout = {3, 0};
    double offset;

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1)
    {
        log_eno("socket\n");
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &rcvtimeout, sizeof(struct timeval));
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(srv_port);
    servaddr.sin_addr.s_addr = inet_host(srv);
    ret = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr));
    if (ret != 0)
    {
        log_eno("connect srv:%s\n", server);
        close(sockfd);
        return -1;
    }

    nbytes = BUFSIZE;

    if (get_ntp_packet(buf, &nbytes) != 0)
    {
        log_err("ntp pkg get err\n");
        close(sockfd);
        return -1;
    }

    send(sockfd, buf, nbytes, 0);

    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    maxfd1 = sockfd + 1;

    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    ret = select(maxfd1, &readfds, NULL, NULL, &timeout);
    if (ret < 0)
    {
        log_eno("select\n");
        close(sockfd);
        return -1;
    }
    else if (ret == 0)
    {
        //log_eno("select timeout\n");
        close(sockfd);
        return -1;
    }

    if (FD_ISSET(sockfd, &readfds))
    {
        if ((nbytes = recv(sockfd, buf, BUFSIZE, 0)) < 0)
        {
            log_eno("recv\n");
            close(sockfd);
            return -1;
        }

        // printf("nbytes = %d\n", nbytes);
        // print_ntp((struct ntphdr *) buf);
        gettimeofday(&recvtv, NULL);
        offset = get_offset((struct ntphdr *)buf, &recvtv);

        gettimeofday(&tv, NULL);

        tv.tv_sec += (int)offset;
        tv.tv_usec += ((offset - (int)offset) * 1000000);

        tv.tv_sec += (tv.tv_usec / 1000000);
        tv.tv_usec = (tv.tv_usec % 1000000);

        if (tv.tv_usec < 0)
        {
            tv.tv_sec--;
            tv.tv_usec += 1000000;
        }

        //log_dbg("[ntp] sec: %ld usec: %ld\n", tv.tv_sec, tv.tv_usec);
        print_time("[ntp]", tv.tv_sec, tv.tv_usec / 1000);

        if (settimeofday(&tv, NULL) != 0)
        {
            log_eno("settimeofday\n");
            close(sockfd);
            return -1;
        }
    }

    close(sockfd);
    return 0;
}

