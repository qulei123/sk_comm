/* 
 * 文件名称：sk_timer.c
 * 摘     要：定时器
 *  
 * 修改历史   版本号         Author   修改内容
 *--------------------------------------------------
 * 2022.10.14   v1      ql     创建文件
 * 2022.11.01   v2      ql     使用有序链表重写定时器模块
 * 2022.11.07   v2.1    ql     加入休眠唤醒机制sem
 * 2022.11.10   v2.2    ql     加入重复定时器
 * 2022.11.28   v2.3    ql     每天某个时刻定时器
 * 2023.03.01   v2.4    ql     添加更新定时器时间功能
 * 2023.07.13   v2.5    ql     更新定时器时间，定时器已经销毁则重新创建
 * 2023.08.14   v2.6    ql     更新定时器时间，需要重新排序
 *--------------------------------------------------
 * note：
 * 1. bug：00:00:00, 无法触发，改为00:00:01
 * 2. bug：当天某时刻的重复定时器已经触发过，修改系统时间为该定时器的触发时间，该定时器无法触发。
 * 3. bug：定时器id溢出后，重新申请的id，可能是之前循环定时器的id(后面解决)
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <semaphore.h>

#include "sk_timer.h"

static struct list_entry list_head;             /* 运行时间链表 */
static struct list_entry list_head_plan;        /* 计划任务链表 */

static pthread_mutex_t lock;
//static sem_t sem;

/*
 * @brief: 函数描述: 获取某一天00:00:00 以来的相对时间: 总秒数
 * @ret  : sk_time_t(某一天00:00:00 以来的时间(单位:s))
 */
static sk_time_t get_sec_of_day(void)
{
    sk_time_t  rel_sec;
    time_t tTm;
    struct tm tDateTime = {0};
 
    tTm = time(NULL);
    tzset();
    localtime_r(&tTm, &tDateTime);

    /* 计算当天的相对(00:00:00)以来的总秒数 */
    rel_sec = tDateTime.tm_sec + tDateTime.tm_min * 60
              + tDateTime.tm_hour * 60 * 60;

    return rel_sec;
}

uint32_t ctime_to_sec(char *time)
{
    int hour, min, sec;
    uint32_t rel_sec;
    int ret;
    
    ret = sscanf(time, "%02d:%02d:%02d", &hour, &min, &sec);
    if (ret != 3)
    {
        printf("time fomart[%s] invalid\n", time);
        return 0;
    }

    rel_sec = hour * 3600 + min * 60 + sec;
    if (rel_sec == 0)
    {
        /* bug：00:00:00, 无法触发，改为00:00:01 */
        rel_sec = 1;
    }
    
    return rel_sec;
}

static void plan_task(sk_timer_t *tmr, void *arg)
{
    sk_timer_t *timer, *next;
    sk_time_t  sec;

    if (list_empty(&list_head_plan))
    {
        return;
    }

    /* 需要处理done，必须遍历所有定时器，所以排序没有作用了 */
    sec = get_sec_of_day();
    list_for_each_entry_safe(timer, next, &list_head_plan, list)
    {
        //printf("plan time id %d\n", timer->id);
        if (timer->sec <= sec)
        {        
            /* 时间到期 */
            if (timer->done == 0)
            {
                timer->handler(timer, timer->arg);
                if (timer->type == TIMER_TYPE_ONCE)
                {
                    list_remove(&timer->list);
                    free(timer);
                    timer = NULL;
                }
                else
                {
                    timer->done = 1;
                }
            }
        }
        else
        {
            /* bug：00:00:00, 无法清零 */
            timer->done = 0;
        }
    }
}

static uint32_t get_run_time(void)
{
    struct timespec  ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* id溢出后，获取到的id可能会重复，暂时来看影响不大 */
static int get_timer_id(void)
{
    static int id = 0;
    int cur_id;
    
    pthread_mutex_lock(&lock);
    cur_id = id++;
    if (id < 0)
    {
        id = 1;     /* 定时器id溢出后，不要占用id为0的定制器(本模块自用定时器) */
    }
    pthread_mutex_unlock(&lock);

    return cur_id;
}

/* 插入有序链表 */
static void add_timer_to_seqlist(struct list_entry *head, sk_timer_t *timer)
{
    sk_timer_t *entry;

    /* 找到第一个大于timer的节点 */
    list_for_each_entry(entry, head, list)
    {
        if (entry->sec > timer->sec)
        {
            break;
        }
        
        if ((entry->sec == timer->sec) && 
            (entry->msec >= timer->msec))
        {
            break;
        }
    }

    /* 分别加入2个链表 */
    list_insert_before(&timer->list, &entry->list);

#if 0   
    /* 是否需要唤醒定时器处理 */
    if (list_only_one(head))
    {
        printf("only one timer\n");
        sem_post(&sem);
    }
#endif

}

static int sk_timer_find(uint32_t *off_msec)
{
    sk_timer_t *timer;
    int32_t msec;

    if (list_empty(&list_head))
    {
        return -1;
    }

    timer = list_first_entry(&list_head, sk_timer_t, list);
    msec  = timer->msec - get_run_time();
    *off_msec  = (uint32_t)(msec > 0 ? msec : 0);

    return 0;
}

static void sk_timer_update(sk_timer_t *timer)
{  
    list_remove(&timer->list);
    if (timer->type == TIMER_TYPE_ONCE)
    {
        free(timer);
        timer = NULL;
    }
    else if (timer->type == TIMER_TYPE_KEEP)
    {
        timer->sec  = 0;
        timer->msec = get_run_time() + timer->interval;
        add_timer_to_seqlist(&list_head, timer);
    } 
    else
    {
        printf("timer type %d invalid\n", timer->type);
    }
}

static void sk_timer_expire(void)
{
    sk_timer_t *timer, *next;

    /* 可能存在多个时间到期的定时器 */
    list_for_each_entry_safe(timer, next, &list_head, list)
    {
        if (timer->msec > get_run_time())
        {
            /* 时间没有到期 */
            break;
        }

        timer->handler(timer, timer->arg);
        sk_timer_update(timer);
    }
}

sk_timer_t *sk_timer_find_by_id(int id)
{
    sk_timer_t *timer;

    list_for_each_entry(timer, &list_head, list)
    {
        if(timer->id == id)
        {
            return timer;
        }
    }

    list_for_each_entry(timer, &list_head_plan, list)
    {
        if(timer->id == id)
        {
            return timer;
        }
    }
    
    return NULL;
}

void sk_timer_log(void)
{
    sk_timer_t *timer;
    
    list_for_each_entry(timer, &list_head, list)
    {
        printf("timer id %d, msec %u\n", timer->id, timer->msec);
    }

    list_for_each_entry(timer, &list_head_plan, list)
    {
        printf("timer id %d, sec %u\n", timer->id, timer->sec);
    }
}

static void *timer_thread(void *pdata)
{
    uint32_t msec;
    int ret;

    prctl(PR_SET_NAME, "timer");

    while (1)
    {
        ret = sk_timer_find(&msec);
        if (0 == ret)
        {
            if (msec > 0)
            {
                usleep(msec * 1000);
            }
            sk_timer_expire();
        }
        else
        {
            /* 休眠 */
            //printf("timer sleep\n");
            //sem_wait(&sem);
            usleep(500);
        }
    }

    return NULL;
}

int sk_timer_init(void)
{
    pthread_t tThdId;
    int ret;

    list_init(&list_head);
    list_init(&list_head_plan);
    
    pthread_mutex_init(&lock, NULL);

    /* 默认启动一个1s的循环定时器，所以不需要休眠唤醒 */
    //sem_init(&sem, 0, 0);

    ret = pthread_create(&tThdId, NULL, timer_thread, NULL);
    if (0 != ret)
    {
        printf("pthread_create\n");
        return -1;
    }

    ret = pthread_detach(tThdId);
    if (0 != ret)
    {
        printf("pthread_detach\n");
        return -1;
    }

    /* 计划任务：后期可以做成一个模块，从定时器中独立出去 */
    sk_timer_add_keep(1000, plan_task, NULL);

    return 0;
}

/* 更新定时器时间 */
static int sk_timer_uptime(int id, uint32_t sec, uint32_t msec)
{
    sk_timer_t *timer;
    struct list_entry *head;

    /* id:0, 时间不能修改 */
    if (id <= 0)
    {
        //printf("timer id:%d invalid\n", id);
        return -1;
    }

    timer = sk_timer_find_by_id(id);
    if (NULL == timer)
    {
        //printf("not find timer %d\n", id);
        return -1;
    }

    /* 修改时间 */
    list_remove(&timer->list);
    if (((TIMER_TYPE_ONCE == timer->type)) || 
         (TIMER_TYPE_KEEP == timer->type))
    {
        timer->sec      = 0; 
        timer->msec     = get_run_time() + msec;
        timer->interval = msec;
        head            = &list_head;
    }
    else
    {
        timer->sec      = sec;
        timer->msec     = 0;
        timer->interval = 24 * 3600;
        timer->done     = 0;
        if (timer->sec < get_sec_of_day())
        {        
            /* 今天已经执行过了 */
            timer->done = 1;
        }
        head            = &list_head_plan;
    }
    add_timer_to_seqlist(head, timer);

    return id;
}

static int sk_timer_add(timer_type_t type, uint32_t sec, uint32_t msec, timer_handler_t handler, void *arg)
{
    sk_timer_t *timer;
    struct list_entry *head;

    timer = (sk_timer_t *)malloc(sizeof(*timer));
    if (NULL == timer)
    {
        perror("malloc\n");
        return -1;
    }

    timer->type     = type;
    timer->id       = get_timer_id();
    if (((TIMER_TYPE_ONCE == type)) || 
         (TIMER_TYPE_KEEP == type))
    {
        timer->sec      = 0; 
        timer->msec     = get_run_time() + msec;
        timer->interval = msec;
        head            = &list_head;
    }
    else
    {
        timer->sec      = sec;
        timer->msec     = 0;
        timer->interval = 24 * 3600;
        head            = &list_head_plan;
        timer->done     = 0;
        if (timer->sec < get_sec_of_day())
        {
            /* 今天已经执行过了 */
            timer->done = 1;
        }
    }
    timer->handler  = handler;
    timer->arg      = (arg != NULL) ? arg : timer;
    add_timer_to_seqlist(head, timer);
    
    //printf("add timer id %d\n", timer->id);

    return timer->id;
}

int sk_timer_add_once(uint32_t msec, timer_handler_t handler, void *arg)
{
    return sk_timer_add(TIMER_TYPE_ONCE, 0, msec, handler, arg);
}

int sk_timer_once_modify(int id, uint32_t msec, timer_handler_t handler, void *arg)
{
    int ret;

    ret = sk_timer_uptime(id, 0, msec);
    if (ret < 0)
    {
        /* 定时器不存在, 重新创建 */
        return sk_timer_add(TIMER_TYPE_ONCE, 0, msec, handler, arg);
    }

    return ret;
}

int sk_timer_add_keep(uint32_t msec, timer_handler_t handler, void *arg)
{
    return sk_timer_add(TIMER_TYPE_KEEP, 0, msec, handler, arg);
}

/* 创建或修改定时器 */
int sk_timer_keep_modify(int id, uint32_t msec, timer_handler_t handler, void *arg)
{
    int ret;

    ret = sk_timer_uptime(id, 0, msec);
    if (ret < 0)
    {
        /* 定时器不存在, 重新创建 */
        return sk_timer_add(TIMER_TYPE_KEEP, 0, msec, handler, arg);
    }

    return ret;
}

/* 时间格式：00:00:00 */
int sk_timer_add_day(char *time, timer_handler_t handler, void *arg)
{
    uint32_t rel_sec;
    
    rel_sec = ctime_to_sec(time);
    if (rel_sec == 0)
    {
        return -1;
    }

    return sk_timer_add(TIMER_TYPE_ONCE, rel_sec, 0, handler, arg);
}

int sk_timer_day_modify(int id, char *time, timer_handler_t handler, void *arg)
{
    uint32_t rel_sec;
    int ret;
    
    rel_sec = ctime_to_sec(time);
    if (rel_sec == 0)
    {
        return -1;
    }

    ret = sk_timer_uptime(id, rel_sec, 0);
    if (ret < 0)
    {
        /* 定时器不存在, 重新创建 */
        return sk_timer_add(TIMER_TYPE_ONCE, rel_sec, 0, handler, arg);
    }

    return ret;
}

/* 时间格式：00:00:00 */
int sk_timer_add_day_keep(char *time, timer_handler_t handler, void *arg)
{
    uint32_t rel_sec;

    rel_sec = ctime_to_sec(time);
    if (rel_sec == 0)
    {
        return -1;
    }

    return sk_timer_add(TIMER_TYPE_DAY, rel_sec, 0, handler, arg);
}

int sk_timer_day_keep_modify(int id, char *time, timer_handler_t handler, void *arg)
{
    uint32_t rel_sec;
    int ret;

    rel_sec = ctime_to_sec(time);
    if (rel_sec == 0)
    {
        return -1;
    }

    ret = sk_timer_uptime(id, rel_sec, 0);
    if (ret < 0)
    {
        /* 定时器不存在, 重新创建 */
        return sk_timer_add(TIMER_TYPE_DAY, rel_sec, 0, handler, arg);
    }

    return ret;
}

int sk_timer_del(int id)
{
    sk_timer_t *timer;

    timer = sk_timer_find_by_id(id);
    if (NULL == timer)
    {
        //printf("not find d timer %d\n");
        return -1;
    }

    list_remove(&timer->list);
    free(timer);
    //printf("del timer id %d\n", id);

    return 0;
}

static void timer_cb(sk_timer_t *tmr, void *arg)
{
    sk_timer_t *timer = (sk_timer_t *)arg;
    printf("timer handler id %d\n", timer->id);
}

int test1_timer(void)
{
    sk_timer_init();

    sk_timer_add_day_keep("00:00:00", timer_cb, NULL);
    int id = sk_timer_add_day_keep("00:00:10", timer_cb, NULL);
    sk_timer_add_day("00:00:20", timer_cb, NULL);
    sk_timer_add_day_keep("00:00:35", timer_cb, NULL);
    sk_timer_add_day("00:00:35", timer_cb, NULL);
    sk_timer_log();

    sk_timer_del(id);
    sk_timer_log();

    return 1;
}

int test_timer(void)
{
    sk_timer_init();

    sk_timer_add_once(100, timer_cb, NULL);
    sk_timer_add_keep(1000, timer_cb, NULL);
    sk_timer_add_once(500, timer_cb, NULL);
    sk_timer_add_once(90, timer_cb, NULL);
    sk_timer_add_once(900, timer_cb, NULL);
    sk_timer_add_once(2000, timer_cb, NULL);
    sk_timer_add_once(500, timer_cb, NULL);
    sk_timer_add_once(10, timer_cb, NULL);
    sk_timer_log();


    sleep(1);
    sk_timer_log();
    sleep(1);
    sk_timer_log();

    sleep(10);
    sk_timer_add_keep(5000, timer_cb, NULL);

    return 1;
}

int test2_timer(void)
{
    sk_timer_init();
    int id = 0;
    int id1 = 0;
    
    id = sk_timer_once_modify(id, 2000, timer_cb, NULL);
    id1 = sk_timer_once_modify(id1, 10000, timer_cb, NULL);
    sleep(1);
    sk_timer_log();
    printf("-----------%d\n", id);
    id = sk_timer_once_modify(id, 3000, timer_cb, NULL);
    sleep(2);
    sk_timer_log();
    printf("-----------%d\n", id);
    id = sk_timer_once_modify(id, 4000, timer_cb, NULL);
    sk_timer_log();
    printf("-----------%d\n", id);
    sleep(5);
    sk_timer_del(id1);
    sk_timer_log();

    printf("------------------------\n");
    id = 0;
    id = sk_timer_day_keep_modify(id, "00:00:10", timer_cb, NULL);
    sleep(10);
    id = sk_timer_day_keep_modify(id, "00:00:20", timer_cb, NULL);
    sk_timer_log();

    return 1;
}


