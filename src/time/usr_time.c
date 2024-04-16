
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "usr_time.h"


/********************************************************/
/* 作用: 测试接口的调用时间                            */
/********************************************************/
static struct timeval stStartTime;

void record_start_time(void)
{
    gettimeofday(&stStartTime, NULL);
}

unsigned int calc_use_time_ms(const char* func)
{
    struct timeval stEndTime;
    unsigned int time;

    gettimeofday(&stEndTime, NULL);
    time = (stEndTime.tv_sec - stStartTime.tv_sec) * 1000 + (stEndTime.tv_usec - stStartTime.tv_usec) / 1000;

    if (NULL != func)
    {
        log_dbg("%s use time: %dms\n", func, time);
    }

    return time;
}

unsigned int calc_use_time_us(const char* func)
{
    struct timeval stEndTime;
    unsigned int time;

    gettimeofday(&stEndTime, NULL);
    time = (stEndTime.tv_sec - stStartTime.tv_sec) * 1000000 + (stEndTime.tv_usec - stStartTime.tv_usec);

    if (NULL != func)
    {
        log_dbg("%s use time: %dus\n", func, time);
    }

    return time;
}

void get_timeofday(struct timeval *ptCurTime)
{
    gettimeofday(ptCurTime, NULL);
}

/* 返回自1970-01-01 00:00:00到现在经历的ms */
long get_cur_time_ms(void)
{
    struct timeval time_tv;

    gettimeofday(&time_tv, NULL);
    
    return time_tv.tv_sec * 1000 + time_tv.tv_usec / 1000;
}

/* 当前运行时间，单位ms */
long get_run_time_ms(void)
{
    struct timespec  ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

#include <time.h>

/********************************************************/
/* 作用: 获取本地时间                                   */
/********************************************************/
static struct tm tTime;

struct tm *get_cur_time(struct tm *ptTime)
{
    time_t cur_time;
    struct tm *ptTm = (NULL == ptTime) ? &tTime : ptTime;
    struct tm* cur_tm;

#if 1
    time(&cur_time);
    tzset();            /* 解决时区问题 */

    cur_tm = localtime_r(&cur_time, ptTm);
    if (NULL == cur_tm)
    {
        return NULL;
    }
#else
    time(&cur_time);
    cur_tm = localtime(&cur_time);
    if (NULL == cur_tm)
    {
        return NULL;
    }
    memcpy(ptTm, cur_tm, sizeof(*ptTm));
#endif

    return ptTm;
}

char *format_time(E_TimeType eType, struct tm *ptTime)
{
    static char time[64];
    char const *week[] = {"星期天", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
    struct tm *ptTm = (NULL == ptTime) ? &tTime : ptTime;

    if (TM_HHMM == eType)
    {
        snprintf(time, sizeof(time), "%02d:%02d",  ptTm->tm_hour, ptTm->tm_min);                                /* 自动添加`\0` */
    }
    else if (TM_HHMMSS == eType)
    {
        snprintf(time, sizeof(time), "%02d:%02d:%02d",  ptTm->tm_hour, ptTm->tm_min, ptTm->tm_sec);             /* 自动添加`\0` */
    }
    else if (TM_YYMMDD_H == eType)
    {
        snprintf(time, sizeof(time), "%d-%02d-%02d", ptTm->tm_year + 1900, ptTm->tm_mon + 1, ptTm->tm_mday);    /* 自动添加`\0` */
    }
    else if (TM_YYMMDD_P == eType)
    {
        snprintf(time, sizeof(time), "%d.%02d.%02d", ptTm->tm_year + 1900, ptTm->tm_mon + 1, ptTm->tm_mday);    /* 自动添加`\0` */
    }
    else if (TM_YYMMDD_S == eType)
    {
        snprintf(time, sizeof(time), "%d/%02d/%02d", ptTm->tm_year + 1900, ptTm->tm_mon + 1, ptTm->tm_mday);    /* 自动添加`\0` */
    }
    else if (TM_YYMMDD_HHMMSS == eType)
    {
        /* 当文件名使用，不能有: */
        snprintf(time, sizeof(time), "%04d-%02d-%02d_%02d-%02d-%02d", ptTm->tm_year + 1900, ptTm->tm_mon + 1, ptTm->tm_mday, 
                 ptTm->tm_hour, ptTm->tm_min, ptTm->tm_sec);                                                    /* 自动添加`\0` */
    }
    else if (TM_YYMMDD_HHMMSS_1 == eType)
    {
        snprintf(time, sizeof(time), "%04d-%02d-%02d %02d:%02d:%02d", ptTm->tm_year + 1900, ptTm->tm_mon + 1, ptTm->tm_mday, 
                 ptTm->tm_hour, ptTm->tm_min, ptTm->tm_sec);                                                    /* 自动添加`\0` */
    }
    else if (TM_YYMMDD_HHMMSS_W == eType)
    {
        snprintf(time, sizeof(time), "%04d-%02d-%02d %02d:%02d:%02d %s", ptTm->tm_year + 1900, ptTm->tm_mon + 1, ptTm->tm_mday, 
                 ptTm->tm_hour, ptTm->tm_min, ptTm->tm_sec, week[ptTm->tm_wday]);                               /* 自动添加`\0` */
    }
    else
    {
        log_info("time type %d invalid\n", eType);
        strcpy(time, "1970-01-01 00:00:00");
    }

    return time;
}

void print_time(const char *tag, time_t sec, int msec)
{
    char buf[64];
    struct tm *timeinfo;

    timeinfo = localtime(&sec);
    strftime(buf, sizeof(buf), "%F %T %z", timeinfo);

    log_info("%s %s, msec:%d\n", tag, buf, msec);
}

void print_curtime(void)
{
    time_t cur_time;
    struct tm* cur_tm;
    char buf[32];

    time(&cur_time);
    cur_tm = localtime(&cur_time);
    if (NULL == cur_tm)
    {
        return;
    }
    strftime(buf, sizeof(buf), "%F %T %z", cur_tm);
    log_info("%s\n", buf);
}

/*  */
int calc_time_in(char *time_min, char *time_max)
{
    assert(NULL != time_min);
    assert(NULL != time_max);
    struct tm cur;
    unsigned int time_min_sec, time_max_sec, cur_sec;
    int hour, min, sec;
        
    sscanf(time_min, "%d:%d:%d", &hour, &min, &sec);
    time_min_sec = 3600 * hour + 60 * min + sec;
    //printf("time_min_sec, %d-%d-%d\n", hour, min, sec);
    
    sscanf(time_max, "%d:%d:%d", &hour, &min, &sec);
    time_max_sec = 3600 * hour + 60 * min + sec;
    //printf("time_max_sec, %d-%d-%d\n", hour, min, sec);
    
    get_cur_time(&cur);
    cur_sec = 3600 * cur.tm_hour + 60 * cur.tm_min + cur.tm_sec;

    if (time_min_sec <= time_max_sec)
    {
        if ((time_min_sec < cur_sec) && (cur_sec < time_max_sec))
        {
            return 1;
        }
    }
    else
    {
        /* 跨天 */
        if ((time_min_sec < cur_sec) || (cur_sec < time_max_sec))
        {
            return 1;
        }
    }

    return 0;
}

