/* 
 * 文件名称：ack_async.c
 * 摘     要：确认消息异步处理(消息重发)
 *  
 * 修改历史   版本号         Author   修改内容
 *--------------------------------------------------
 * 2024.03.15   v1      ql     创建文件
 *                             没有收到确认消息，重发功能
 *--------------------------------------------------
 * note：
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "deftypes.h"
#include "list.h"
#include "ack_async.h"


static int ack_list_node_is_exist(ack_ctx_t* ctx, int cmd, uint32_t msg_id)
{
    struct list_entry *curr, *next;
    ack_cb_t *ack_cb;

    if (list_empty(&ctx->ack_list))
    {
        return 0;
    }

    list_for_each_safe(curr, next, &ctx->ack_list)
    {
        ack_cb = list_entry(curr, ack_cb_t, list);

        if ((msg_id == ack_cb->ack.msg_id) && (cmd == ack_cb->ack.cmd))
        {
            log_war("ack list is exist ack node\n");
            return 1;
        }
    }

    return 0;
}

static ack_cb_t *ack_cb_create(ack_ctx_t *ctx, ack_info_t *ack, uint8_t *buf, int buf_len)
{
    ack_cb_t *ack_cb;

    ack_cb = (ack_cb_t *)malloc(sizeof(ack_cb_t) + buf_len);
    if (NULL == ack_cb)
    {
        log_eno("malloc\n");
        return NULL;
    }

    list_init(&ack_cb->list);
    ack_cb->ack = *ack;
    ack_cb->payload_len = buf_len;
    ack_cb->payload = (uint8_t *)ack_cb + sizeof(ack_cb_t);
    memcpy(ack_cb->payload, buf, buf_len);    /* save the resend data*/

    return ack_cb;
}

static void ack_cb_destroy(ack_cb_t **ack_cb)
{
    ack_cb_t *tmp = *ack_cb;

    list_remove(&tmp->list);
    free(tmp);
    *ack_cb = NULL;
}

int ack_list_record(ack_ctx_t *ctx, ack_info_t *ack, uint8_t *buf, int buf_len)
{
    ENSURE((NULL != ctx) && (NULL != ack));
    ack_cb_t *ack_cb = NULL;

    if (ack_list_node_is_exist(ctx, ack->cmd, ack->msg_id))
    {
        return 1;
    }

    /* create a ack ctrl block */
    ack_cb = ack_cb_create(ctx, ack, buf, buf_len);
    if (NULL == ack_cb)
    {
        log_err("ack cb create fail\n");
        return 0;
    }

    list_append(&ack_cb->list, &ctx->ack_list);

    return 1;
}

int ack_list_unrecord(ack_ctx_t *ctx, int cmd, uint32_t msg_id)
{
    ENSURE(NULL != ctx);
    struct list_entry *curr, *next;
    ack_cb_t *ack_cb;

    if (list_empty(&ctx->ack_list))
    {
        log_war("ack list empty\n");
        return 1;
    }

    list_for_each_safe(curr, next, &ctx->ack_list)
    {
        ack_cb = list_entry(curr, ack_cb_t, list);

        if ((msg_id != ack_cb->ack.msg_id) || (cmd != ack_cb->ack.cmd))
        {
            continue;
        }

        ack_cb_destroy(&ack_cb);
    }

    return 1;
}

void ack_list_print(ack_ctx_t *ctx)
{
    ENSURE(NULL != ctx);
    ack_cb_t *ack_cb;

    list_for_each_entry(ack_cb, &ctx->ack_list, list)
    {
        log_info("ack_cb: cmd-%d, msg_id-%u, re_time-%d, re_num-%u\n", 
                ack_cb->ack.cmd, ack_cb->ack.msg_id, ack_cb->ack.re_time, ack_cb->ack.re_num);
    }
}

static int time_expire(ack_info_t *ack)
{
    return ack->time_cnt >= ack->re_time;
}

static void ack_list_scan(ack_ctx_t *ctx)
{
    struct list_entry *curr, *next;
    ack_cb_t *ack_cb;
    int ret;

    if (list_empty(&ctx->ack_list))
    {
        return;
    }

    list_for_each_safe(curr, next, &ctx->ack_list)
    {
        ack_cb = list_entry(curr, ack_cb_t, list);

        ack_cb->ack.time_cnt++;
        if (!time_expire(&ack_cb->ack))
        {        
            continue;
        }
        else
        {
            ack_cb->ack.time_cnt = 0;
        }

        /* 先判断次数再resend */
        if (ack_cb->ack.re_num <= 0)
        {
            /* 重发次数到达，销毁 */
            ack_cb_destroy(&ack_cb);
            continue;
        }

        ret = ctx->resend(ack_cb);
        if (ret == 1)
        {
            ack_cb->ack.re_num--;
        }
        else
        {
            ack_cb_destroy(&ack_cb);
        }
    }
}

/* 遍历响应链表 */
static void *ack_list_handler(void *arg)
{
    ack_ctx_t *ctx = (ack_ctx_t *)arg;

    while (ctx->scan_time)
    {
        sleep(ctx->scan_time);
        ack_list_scan(ctx);
    }

    ctx->scan_time = -1;
    return NULL;
}

static int ack_handler_resend(ack_cb_t *ack_cb)
{
    log_war("Please let the caller implement the resend function!!!\n");
    return 0;
}

/*
 *  1. 需要知道哪些响应需要重传, resend
 */
ack_ctx_t *ack_async_init(resend_t resend)
{
    pthread_t thrd;
    ack_ctx_t *ctx;
    int ret;

    ctx = (ack_ctx_t *)malloc(sizeof(*ctx));
    if (NULL == ctx)
    {
        log_eno("malloc\n");
        return NULL;
    }

    ctx->resend = (NULL == resend) ? ack_handler_resend : resend;
    list_init(&ctx->ack_list);
    ctx->scan_time = 1;

    ret = pthread_create(&thrd, NULL, ack_list_handler, (void *)ctx);
    if (0 != ret)
    {
        free(ctx);
        log_eno("pthread_create\n");
        return NULL;
    }

    ret = pthread_detach(thrd);
    if (0 != ret)
    {
        free(ctx);
        log_eno("pthread_detach\n");
        return NULL;
    }

    log_info("ack async init ok\n");
    return ctx;
}

void ack_async_exit(ack_ctx_t *ctx)
{
    ENSURE(NULL != ctx);
    struct list_entry *curr, *next;
    ack_cb_t *ack_cb;

    if (!list_empty(&ctx->ack_list))
    {
        list_for_each_safe(curr, next, &ctx->ack_list)
        {
            ack_cb = list_entry(curr, ack_cb_t, list);
            ack_cb_destroy(&ack_cb);
        }
    }

    /* 确保线程退出后free */
    ctx->scan_time = 0;
    while(-1 != ctx->scan_time)
    {
        usleep(20 * 1000);
    }
    free(ctx);

    log_info("ack async exit ok\n");
    return;
}

