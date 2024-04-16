
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h> /* getaddrinfo -> gethostbyname */
#include <resolv.h>

#include "logit.h"
#include "sntpd.h"
#include "sntp_net.h"

extern struct ntp_peers peer;


static int getaddrbyname(char *host, struct sockaddr_storage *ss)
{
    struct addrinfo *result;
    static int netdown = 0;
    struct addrinfo hints;
    struct addrinfo *rp;
    int err;

    if (!host || !ss)
    {
        errno = EINVAL;
        return 1;
    }

    res_init();

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = 0;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    memset(ss, 0, sizeof(struct sockaddr_storage));
    err = getaddrinfo(host, NULL, &hints, &result);
    if (err)
    {
        switch (err)
        {
        case EAI_NONAME:
            LOG("Failed resolving %s, will try again later ...", host);
            break;
        case EAI_AGAIN:
            LOG("Temporary failure resolving %s, will try again later ...", host);
            break;
        default:
            LOG("Error %d resolving %s: %s", err, host, gai_strerror(err));
            break;
        }
        netdown = errno = ENETDOWN;
        return 1;
    }

    /* The first result will be used. IPV4 has higher priority */
    err = 1;
    for (rp = result; rp; rp = rp->ai_next)
    {
        if (rp->ai_family == AF_INET)
        {
            memcpy(ss, (struct sockaddr_in *)(rp->ai_addr), sizeof(struct sockaddr_in));
            err = 0;
            break;
        }
        if (rp->ai_family == AF_INET6)
        {
            memcpy(ss, (struct sockaddr_in6 *)(rp->ai_addr), sizeof(struct sockaddr_in6));
            err = 0;
            break;
        }
    }
    freeaddrinfo(result);

    if (err)
    {
        errno = EAGAIN;
        return 1;
    }

    if (netdown)
    {
        LOG("Network up, resolved address to hostname %s", host);
        netdown = 0;
    }

    return 0;
}

static int setup_receive(int usd, sa_family_t sin_family, uint16_t port)
{
    struct sockaddr_in6 sin6;
    struct sockaddr_in sin;
    struct sockaddr *sa;
    socklen_t len;

    if (sin_family == AF_INET)
    {
        /* IPV4 */
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
        sin.sin_port = htons(port);

        sa = (struct sockaddr *)&sin;
        len = sizeof(sin);
    }
    else
    {
        /* IPV6 */
        memset(&sin6, 0, sizeof(struct sockaddr_in6));
        sin6.sin6_family = AF_INET6;
        sin6.sin6_port = htons(port);
        sin6.sin6_addr = in6addr_any;

        sa = (struct sockaddr *)&sin6;
        len = sizeof(sin6);
    }

    if (bind(usd, sa, len) == -1)
    {
        ERR(errno, "Failed binding to UDP port %u", port);
        return -1;
    }

    return 0;
}

static int setup_transmit(int usd, struct sockaddr_storage *ss, uint16_t port)
{
    struct sockaddr_in6 *ipv6;
    struct sockaddr_in *ipv4;
    socklen_t len = 0;

    /* Prefer IPv4 over IPv6, for now */
    if (ss->ss_family == AF_INET)
    {
        ipv4 = (struct sockaddr_in *)ss;
        ipv4->sin_port = htons(port);
        len = sizeof(struct sockaddr_in);
    }
    else if (ss->ss_family == AF_INET6)
    {
        ipv6 = (struct sockaddr_in6 *)ss;
        ipv6->sin6_port = htons(port);
        len = sizeof(struct sockaddr_in6);
    }
    else
    {
        ERR(0, "%s: Unsupported address family %d", __func__, ss->ss_family);
        return -1;
    }

    if (connect(usd, (struct sockaddr *)ss, len) == -1)
    {
        ERR(errno, "Failed connecting to NTP server");
        return -1;
    }

    INFO("Connected to NTP server.");
    return 0;
}

/* sntp client */
int setup_socket(char *srv, uint16_t srv_port, uint16_t bind_port)
{
    struct sockaddr_storage ss;
    int sd;

    if (getaddrbyname(srv, &ss))
    {
        if (EINVAL != errno)
            return -1;

        ERR(0, "Unable to look up %s address", srv ?: "<nil>");
        return -1;
    }

    /* open socket based on the server address family */
    if (ss.ss_family != AF_INET && ss.ss_family != AF_INET6)
    {
        ERR(0, "%s: Unsupported address family %d", __func__, ss.ss_family);
        return -1;
    }

    sd = socket(ss.ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (sd == -1)
        return -1;

    if (setup_receive(sd, ss.ss_family, bind_port) ||
        setup_transmit(sd, &ss, srv_port))
    {
        close(sd);
        errno = ENETDOWN;
        return -1;
    }

    /*
     * Every day: reopen socket and perform a new DNS lookup.
     */
    alarm(60 * 60 * 24);

    return sd;
}


/* sntp server */
extern size_t compose_reply(struct ntp *ntp, size_t len);

int server_init(uint16_t port)
{
    int sd;

    sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sd == -1)
        return -1;

    setup_receive(sd, AF_INET, port);

    return sd;
}

static int validate_request(char *buf, size_t len)
{
    struct ntp *ntp = (struct ntp *)buf;
    int vn, mode;

    if (len < sizeof(struct ntp))
    {
        DBG("NTP request too small (%zd)", len);
        return 1;
    }

    /*
     *  0 1 2 3 4 5 6 7
     * +-+-+-+-+-+-+-+-+
     * |LI | VN  |MODE |
     * +-+-+-+-+-+-+-+-+
     */
    //li   = (ntp->flags >> 6) & 0x3;
    vn = (ntp->flags >> 3) & 0x7;
    mode = (ntp->flags >> 0) & 0x7;
    //DBG("flags 0x%01x - li:%d - vn:%d - mode:%d", ntp->flags, li, vn, mode);

    if (mode != 3 && mode != 0)
    { /* client */
        DBG("NTP request not a client (%d), flags: 0x%01x", mode, ntp->flags);
        return 1;
    }

    if (vn != 3 && vn != 4)
    {
        DBG("NTP request has wrong version (%d), flags: 0x%01x", vn, ntp->flags);
        return 1;
    }

    return 0;
}


int server_recv(int sd)
{
    struct sockaddr_storage ss;
    struct ntptime recv_ts, xmit_ts;
    struct ntp *ntp;
    socklen_t ss_len;
    ssize_t num;
    char buf[128];

    ss_len = sizeof(struct sockaddr_storage);
    num = recvfrom(sd, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&ss, &ss_len);
    if (num == -1)
        return -1;

    get_packet_timestamp(sd, &recv_ts);
    if (validate_request(buf, num))
    {
        DBG("Validation failed");
        return -1;
    }

    /* Compose NTP message reply */
    ntp = (struct ntp *)buf;
    num = compose_reply(ntp, sizeof(*ntp));

    memcpy(&ntp->origin_ts, &ntp->xmit_ts, sizeof(ntp->origin_ts));

    ntp->recv_ts.coarse = htonl(recv_ts.coarse);
    ntp->recv_ts.fine = htonl(recv_ts.fine);

    ntp->refclk_ts.coarse = htonl(peer.last_update_ts.coarse);
    ntp->refclk_ts.fine = htonl(peer.last_update_ts.fine);

    ntpc_gettime(&xmit_ts);
    ntp->xmit_ts.coarse = htonl(xmit_ts.coarse);
    ntp->xmit_ts.fine = htonl(xmit_ts.fine);

    return sendto(sd, buf, num, 0, (struct sockaddr *)&ss, ss_len);
}

int check_source(int data_len, struct sockaddr_storage *ss, uint16_t srv_port)
{
    struct sockaddr_in6 *ipv6;
    struct sockaddr_in *ipv4;
    uint16_t port;

    (void)data_len;
#ifdef ENABLE_DEBUG
    DBG("packet of length %d received", data_len);
#endif

    if (ss->ss_family == AF_INET)
    {
        ipv4 = (struct sockaddr_in *)ss;
        port = ntohs(ipv4->sin_port);
    }
    else if (ss->ss_family == AF_INET6)
    {
        ipv6 = (struct sockaddr_in6 *)ss;
        port = ntohs(ipv6->sin6_port);
    }
    else
    {
        ERR(0, "%s: Unsupported address family %d", __func__, ss->ss_family);
        return 1;
    }

    /*
     * we could check that the source is the server we expect, but
     * Denys Vlasenko recommends against it: multihomed hosts get it
     * wrong too often.
     */
    if (NTP_PORT != port)
    {
        if (port != srv_port)
        {
            INFO("%s: invalid port: %u", __func__, port);
            return 1;
        }
    }

    return 0;
}

