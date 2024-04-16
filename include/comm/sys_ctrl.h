
#ifndef _SYS_CTRL_H_
#define _SYS_CTRL_H_

#ifdef __cplusplus
extern "C" {
#endif


#define TIME_NULL       0
#define TIME_LOCAL      1
#define TIME_NTP        2

typedef struct _time_para
{
    int type;
    char timezone[8];
    char time[24];
    char ntp_srv[64];
    uint16_t ntp_port;
    int ntp_intv;
}time_para_t;

void sys_reboot(void);
int sys_set_time(char *dt);
int set_time(time_para_t *time_para);


#ifdef __cplusplus
}
#endif

#endif  /* _SYS_CTRL_H_ */

