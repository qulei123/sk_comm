#ifndef _USR_TIME_H_
#define _USR_TIME_H_

#include "deftypes.h" 

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    TM_HHMM = 0x1,                  /* 14:25 */
    TM_HHMMSS = 0x2,                /* 14:25:26 */
    TM_YYMMDD_H = 0x11,             /* 2022-09-13 */
    TM_YYMMDD_P = 0x12,             /* 2022.09.13 */
    TM_YYMMDD_S = 0x13,             /* 2022/09/13 */
    TM_YYMMDD_HHMMSS   = 0x20,      /* 2022-11-28_14-25-26, 用于文件名，无空格冒号 */
    TM_YYMMDD_HHMMSS_1 = 0x21,      /* 2022-11-28 14:25:26 */
    TM_YYMMDD_HHMMSS_W = 0x25,      /* 2022-11-28 14:25:26 星期一 */
}E_TimeType;

void record_start_time(void);
unsigned int calc_use_time_ms(const char* func);
unsigned int calc_use_time_us(const char* func);
void get_timeofday(struct timeval *ptCurTime);
long get_cur_time_ms(void);
long get_run_time_ms(void);
struct tm *get_cur_time(struct tm *ptTime);
char *format_time(E_TimeType eType, struct tm *ptTime);
void print_time(const char *tag, time_t sec, int msec);
void print_curtime(void);
int calc_time_in(char *time_min, char *time_max);


#ifdef __cplusplus
}
#endif

#endif /* _USR_TIME_H_ */

