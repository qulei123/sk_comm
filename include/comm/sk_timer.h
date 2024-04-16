
#ifndef __SK_TIMER_H__
#define __SK_TIMER_H__

#include <stdint.h>
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef long    sk_time_t;

struct sk_timer;
typedef struct sk_timer sk_timer_t;

typedef void (*timer_handler_t)(sk_timer_t *tmr, void *arg);

/* 时间类型 */
typedef enum
{
    TIME_TYPE_RUN  = 0x1,
    TIME_TYPE_REAL = 0x2,
} time_type_t;

/* 定时器类型 */
typedef enum
{
    TIMER_TYPE_ONCE  = 0x1,
    TIMER_TYPE_KEEP  = 0x2,
    TIMER_TYPE_DAY   = 0x3,     /* 每天某个时刻定时器 */
    TIMER_TYPE_WEEK  = 0x4,
    TIMER_TYPE_MONTH = 0x5,
    TIMER_TYPE_DATE  = 0x6,
} timer_type_t;

typedef struct sk_timer
{
    struct list_entry list;
    timer_type_t type;
    int id;                     /* 从0开始, 0为计划任务定时器 */
    uint32_t sec;               /* 用于大于一天的定时器 */
    uint32_t msec;
    uint32_t interval;
    timer_handler_t handler;
    void *arg;
    int done;       /* 这个标志位没有好的处理办法 */
}sk_timer_t;


int sk_timer_init(void);
int sk_timer_add_once(uint32_t msec, timer_handler_t handler, void *arg);
int sk_timer_add_keep(uint32_t msec, timer_handler_t handler, void *arg);
int sk_timer_add_day(char *time, timer_handler_t handler, void *arg);
int sk_timer_add_day_keep(char *time, timer_handler_t handler, void *arg);
int sk_timer_once_modify(int id, uint32_t msec, timer_handler_t handler, void *arg);
int sk_timer_keep_modify(int id, uint32_t msec, timer_handler_t handler, void *arg);
int sk_timer_day_modify(int id, char *time, timer_handler_t handler, void *arg);
int sk_timer_day_keep_modify(int id, char *time, timer_handler_t handler, void *arg);
int sk_timer_del(int id);
void sk_timer_log(void);
int test_timer(void);
int test1_timer(void);   
int test2_timer(void);  


#ifdef __cplusplus
}
#endif

#endif /* __SK_TIMER_H__ */

