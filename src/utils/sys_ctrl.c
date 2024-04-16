
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#define __USE_XOPEN
#include <time.h>
#include <sys/time.h>
#include <sys/reboot.h>

#include "deftypes.h"
#include "tools.h"
#include "ntp.h"
#include "usr_time.h"
#include "sys_ctrl.h"


void sys_reboot(void)
{
    sync();
    reboot(RB_AUTOBOOT);
}

/* 时间格式: %Y-%m-%d %H:%M:%S */
int sys_set_time(char *dt)
{
    struct tm _tm = {0};
    struct timeval tv = {0};
    time_t timep;

#if 0
    struct tm rtc_time;
    sscanf(dt, "%d-%d-%d %d:%d:%d", &rtc_time.tm_year, &rtc_time.tm_mon, &rtc_time.tm_mday, 
                                    &rtc_time.tm_hour, &rtc_time.tm_min, &rtc_time.tm_sec);
    _tm.tm_sec   = rtc_time.tm_sec;
    _tm.tm_min   = rtc_time.tm_min;
    _tm.tm_hour  = rtc_time.tm_hour;
    _tm.tm_mday  = rtc_time.tm_mday;
    _tm.tm_mon   = rtc_time.tm_mon - 1;
    _tm.tm_year  = rtc_time.tm_year - 1900;
    _tm.tm_isdst = 0;                       /* tm_isdst未初始化，会被设置为随机值，mktime结果可能出现异常 */
#else
    if (strptime(dt, "%Y-%m-%d %H:%M:%S", &_tm) == NULL)
    {  
        log_err("invalid time format[%s]\n", dt);  
        return 0;  
    }
#endif

    timep = mktime(&_tm);
    if (timep == -1)
    {
        log_eno("mktime dst[%d]\n", _tm.tm_isdst);
        return 0;
    }

    tv.tv_sec = timep;
    tv.tv_usec = 0;
    if (settimeofday(&tv, NULL) < 0)
    {
        log_eno("settimeofday\n");
        return 0;
    }

    return 1;
}

int set_time(time_para_t *time_para)
{
    int ret;

    if (TIME_LOCAL == time_para->type)
    {
        ret = sys_set_time(time_para->time);
    }
    else if (TIME_NTP == time_para->type)
    {
        ret = ntp_sync_time(time_para->ntp_srv, time_para->ntp_port);
        ret = ret ? 0 : 1;
    }
    else
    {
        log_err("time type[%d] para invalid\n", time_para->type);
        return 0;
    }

    if (ret)
        system("hwclock -uw");

    print_curtime();
    return ret;
}

