
#ifndef __NTP_H__
#define __NTP_H__

#ifdef __cplusplus
extern "C"
{
#endif


int ntp_sync_time(char *server, uint16_t port);


#ifdef __cplusplus
}
#endif

#endif  /* __NTP_H__ */

