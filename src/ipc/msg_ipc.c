/* 
 * 文件名称：msg_ipc.c
 * 摘     要：进程通信消息队列
 *  
 * 修改历史   版本号         Author   修改内容
 *--------------------------------------------------
 * 2022.09.16   v1      ql     创建文件
 * 2022.10.24   v2      ql     合入同步消息机制
 * 2023.01.17   v2.1    ql     同步消息发送失败，清空队列
 * 
 *--------------------------------------------------
 */
#include <mqueue.h>
#include <stdbool.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <event2/event.h>
#include <event2/event_struct.h>

#include "deftypes.h"
#include "msg_ipc.h"

#define IPC_CORE

#define IPC_CTW        "/core2web"
#define IPC_WTC        "/web2core"
#define IPC_CTW_SYNC   "/core2web_sync"
#define IPC_WTC_SYNC   "/web2core_sync"

static mqd_t mqid_send;
static mqd_t mqid_recv;
static char recv_buf[MQ_BUF_SIZE];

static mqd_t mqid_send_sync;
static mqd_t mqid_recv_sync;


/* -----------------------------------------------------------------------------
 * 异步处理
 */
static void msg_handler(char *buf, unsigned int size)
{
    log_info("msg_handler:\n");
    log_hex(buf, size);
}

int msg_ipc_put(char *buf, unsigned int size) 
{
    return mq_send(mqid_send, buf, size, MSG_IPC_PRIO_ASYNC);
}

/* 消息异步处理，不要使用该函数 */
int msg_ipc_get(char *buf, unsigned int size) 
{
    unsigned int prio;
    return mq_receive(mqid_recv, buf, size, &prio);
}

/* 接收异步消息，并处理 */
static void mq_msg_cb(int fd, short event, void *arg)
{
    msg_ipc_handler_t handler = (NULL != arg) ? (msg_ipc_handler_t)arg : msg_handler;
    unsigned int prio;
    ssize_t n;

#if 0    
    struct mq_attr attr;
    if(mq_getattr(g_mqid, &attr) == -1)
    {
        printf("get attr error\r\n");
        return;
    }

    if(attr.mq_curmsgs == (long)0)
    {
        printf("no messages in queue\r\n");
        return;
    }
    printf("mq_curmsgs %d \r\n", attr.mq_curmsgs);
#endif

    memset(recv_buf, 0, sizeof(recv_buf));
    n = mq_receive((mqd_t)fd, recv_buf, sizeof(recv_buf), &prio);
    if (n == -1)
    {
        log_eno("mq_receive\n");
        return;
    }
    
    handler(recv_buf, n);
}

/* -----------------------------------------------------------------------------
 * 同步处理
 */

/* 清空同步接收队列 */
static void msg_ipc_flush(mqd_t mqid)
{
    struct mq_attr attr;    
    char recv_buf[MQ_BUF_SIZE];
    unsigned int i, prio;
    
    if(mq_getattr(mqid, &attr) == -1)
    {
        log_eno("get attr\n");
        return;
    }

    for (i = 0; i < attr.mq_curmsgs; i++)
    {
        mq_receive(mqid, recv_buf, sizeof(recv_buf), &prio);
    }
}

static void mq_read_sync(int fd, short event, void *arg)
{
    resp_msg_t *resp = (resp_msg_t *)arg;
    unsigned int prio;
    int len;

    if (!(event & EV_READ))
    {
        return;
    }

    len = mq_receive(fd, resp->buf, MQ_BUF_SIZE, &prio);
    if (len == -1)
    {
        log_eno("mq_receive\n");
        return;
    }
    
    resp->len = len;
    
    return;
}

/* 同步消息: 发送使用异步节点, 同步节点接收
 * 1. 消息内容中指明此消息为同步消息
 * 2. 使用异步消息处理的回调函数处理
 * 3. 避免等待同步消息时，对方发送了一条其他的同步消息，导致同步消息乱序(所以使用发送使用异步节点，同步节点只接收数据)
 * 4. 需要使用高优先级发送同步消息，为了减少异步消息的处理时间，处理过程请采用线程处理
 * 5. 使用 mq_timedreceive, 系统时间被修改会影响阻塞时间
 */
int msg_ipc_send_sync(char *buf, unsigned int size, resp_msg_t *resp, unsigned int timeout)
{
    int ret;

    if (resp->len < MQ_BUF_SIZE)
    {
        log_err("recv buf len(%d),should >= %d\n" , resp->len, MQ_BUF_SIZE);
        return -1;
    }
    
    msg_ipc_flush(mqid_recv_sync);
    ret = mq_send(mqid_send, buf, size, MSG_IPC_PRIO_SYNC);     /* 使用高优先级发送该消息 */
    if (ret == -1)
    {
        log_eno("mq_send sync\n");
        if (errno == EAGAIN)
        {
            /* 队列满，清空队列 */
            log_err("send que full\n");
            msg_ipc_flush(mqid_send);
        }
        return ret;
    }

#if 1
    struct event_base* base;
    struct event *evmq;
    struct timeval tv;

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    base = event_base_new();
    evmq = event_new(base, mqid_recv_sync, EV_READ | EV_TIMEOUT, mq_read_sync, (void *)resp);
    event_add(evmq, &tv);
    event_base_loop(base, EVLOOP_ONCE);
    if (evmq->ev_res & EV_TIMEOUT)
    {
        ret = -1;
    }
    
    event_free(evmq);
    event_base_free(base);
#else
    unsigned int prio;
    struct timespec tm;
    long long nsec;     /* 64 bit */

    clock_gettime(CLOCK_REALTIME, &tm);
    nsec = tm.tv_nsec + (long long)timeout * 1000000;
    tm.tv_sec += nsec / 1000000000;
    tm.tv_nsec = nsec % 1000000000;
    ret = mq_timedreceive(mqid_recv_sync, resp->buf, MQ_BUF_SIZE, &prio, &tm);      /* 需要阻塞接收 */
    if (ret == -1)
    {
        log_eno("mq_timedreceive\n");
        return ret;
    }
    resp->len = ret;
    ret = 0;
#endif

    return ret;
}

/* 回复消息同步，使用该函数 */
int msg_ipc_resp_sync(char *buf, unsigned int size) 
{
    return mq_send(mqid_send_sync, buf, size, MSG_IPC_PRIO_SYNC);
}


/* -----------------------------------------------------------------------------
 * init
 */
static void *msg_ipc_thread(void *pdata)
{
    struct event_base* base;
    struct event *evmq;
    
    prctl(PR_SET_NAME, "msg_ipc");
    
    base = event_base_new();
    evmq = event_new(base, mqid_recv , EV_READ | EV_PERSIST, mq_msg_cb, pdata);           /* 异步处理 */    
    event_add(evmq, NULL);
    event_base_dispatch(base);

    return 0;
}

static mqd_t msg_ipc_mq_open(char *name, int mq_size, int flag)
{
    mqd_t mqid;
    int flags = O_RDWR | O_CREAT | O_EXCL | flag;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    struct mq_attr attr;
    
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = mq_size;
    attr.mq_msgsize = sizeof(recv_buf);
    attr.mq_curmsgs = 0;
    
    //mq_unlink(name);
    mqid = mq_open(name, flags, mode, &attr);
    if (mqid == -1)
    {
        if (EEXIST == errno)
        {
            mqid = mq_open(name, O_RDWR | flag);
            if (mqid == -1)
            {
                log_eno("mq_open\n");
                return -1;
            }
        }
        else
        {
            log_eno("mq_open\n");
            return -1;
        }
    }

    return mqid;
}

int msg_ipc_init(msg_ipc_handler_t handler)
{
    int ret;
    pthread_t tThdId;

    /* 使用mq_timedreceive阻塞接收，配置阻塞方式 */
#ifdef IPC_CORE
    mqid_send = msg_ipc_mq_open(IPC_CTW, MQ_MSG_SIZE, O_NONBLOCK);
    mqid_recv = msg_ipc_mq_open(IPC_WTC, MQ_MSG_SIZE, O_NONBLOCK);
    mqid_send_sync = msg_ipc_mq_open(IPC_CTW_SYNC, 3, O_NONBLOCK);
    mqid_recv_sync = msg_ipc_mq_open(IPC_WTC_SYNC, 3, O_NONBLOCK);      
#else
    mqid_send = msg_ipc_mq_open(IPC_WTC, MQ_MSG_SIZE, O_NONBLOCK);
    mqid_recv = msg_ipc_mq_open(IPC_CTW, MQ_MSG_SIZE, O_NONBLOCK);
    mqid_send_sync = msg_ipc_mq_open(IPC_WTC_SYNC, 3, O_NONBLOCK);
    mqid_recv_sync = msg_ipc_mq_open(IPC_CTW_SYNC, 3, O_NONBLOCK);
#endif

    /* 缓存消息有用，这里不能清空消息队列 */
    
    ret = pthread_create(&tThdId, NULL, msg_ipc_thread, handler);
    if (0 != ret)
    {
        log_eno("pthread_create\n");
        return -1;
    }

    ret = pthread_detach(tThdId);
    if (0 != ret)
    {
        log_eno("pthread_detach\n");
        return -1;
    }
    
    printf("msg ipc init ok, id %d, %d\n", mqid_send, mqid_recv);

    return 0;
}

