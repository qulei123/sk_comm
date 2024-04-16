/* 
 * 文件名称：msg_que.c
 * 摘     要：线程通信消息队列
 *  
 * 修改历史   版本号         Author   修改内容
 *--------------------------------------------------
 * 2022.07.06   v1      ql     创建文件
 * 2022.07.08   v1.1    ql     加入消息类型字段 
 * 2022.10.10   v1.2    ql     加入消息类型队列是否满
 *--------------------------------------------------
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#include "msg_que.h"

msg_que_t *msg_que_create(uint32_t len_max)
{
    msg_que_t *que;
    
    que = malloc(sizeof(*que));
    if (!que)
    {
        perror("malloc failed.");
        return NULL;
    }

    memset(que, 0, sizeof(*que));
    que->que = NULL;
    que->que_end = NULL;
    que->len = 0;
    que->len_max = len_max;
    
    pthread_mutex_init(&que->mutex, NULL);
    pthread_cond_init(&que->cond_get, NULL);
    pthread_cond_init(&que->cond_put, NULL);

    return que;
}

void msg_que_destroy(msg_que_t *que)
{
    if (!que)
    {
        return;
    }

    /* 清空队列 */
    msg_que_flush(que);
    
    pthread_cond_destroy(&que->cond_put);
    pthread_cond_destroy(&que->cond_get);
    pthread_mutex_destroy(&que->mutex);
    free(que);
}

#if 1

int msg_que_put(msg_que_t *que, void *data)
{
    int type = 0;
    return msg_que_put_with_type(que, type, data);
}

int msg_que_put_with_type(msg_que_t *que, int type, void *data)
{
    assert(NULL != que);
    int ret = 0;
    int wait = 1;   /* 默认等待 */
    msg_t *msg;

    pthread_mutex_lock(&que->mutex);

    while (1)
    {
        if (que->len < que->len_max)
        {
            msg = malloc(sizeof(*msg));
            if (msg)
            {
                msg->type = type;
                msg->data = data;
                msg->next = NULL;
                
                /* 入队列 */
                if(que->que == NULL)
                {
                    que->que = msg;
                }
                else
                {
                    que->que_end->next = msg;
                }
                que->que_end = msg;
                que->len++;
                //printf("que->len %d\n", que->len);
                pthread_cond_signal(&que->cond_get);
            }
            else
            {
                perror("malloc failed.");
                ret = -ENOMEM;
            }

            break;
        }
        else
        {
            /* 队列满 */
            if (wait)
            {
                pthread_cond_wait(&que->cond_put, &que->mutex);
            }
            else
            {
                printf("msg que full.\n");
                ret = -1;
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&que->mutex);

    return ret;
}

int msg_que_get(msg_que_t *que, void **data)
{
    int type = 0;
    return msg_que_get_with_type(que, &type, data);
}

int msg_que_get_with_type(msg_que_t *que, int *type, void **data)
{
    assert(NULL != que);
    assert(NULL != data);
    int ret = 0;
    int wait = 1;   /* 默认等待 */
    msg_t *msg;
  
    pthread_mutex_lock(&que->mutex);

    while (1)
    {
        if (que->len > 0)
        {
            /* 出队列 */
            msg = que->que;
            if(msg)
            {
                que->que = msg->next;   // 更新队列头
                if(que->que == NULL)
                {
                    que->que_end = NULL;
                }
                que->len--;
                
                *type = msg->type; 
                *data = msg->data;
                free(msg);
                pthread_cond_signal(&que->cond_put);
                break;
            }
            else
            {
                printf("start msg empty.\n");
                que->len = 0;
            }
            
        }
        else
        {
            /* 队列空 */
            if (wait)
            {
                pthread_cond_wait(&que->cond_get, &que->mutex);
            }
            else
            {
                printf("msg que empty.\n");
                *type = 0; 
                *data = NULL;
                ret = -1;
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&que->mutex);
    
    return ret;
}

#else

int msg_que_put(msg_que_t *que, void *data)
{
    int wait = 1;   /* 默认等待 */
    msg_t *msg;
    
    pthread_mutex_lock(&que->mutex);

    /* 队列满 */
    while (que->len >= que->len_max)
    {
        if (wait)
        {
            pthread_cond_wait(&que->cond_put, &que->mutex);
        }
        else
        {
            printf("msg que full.\n");
            pthread_mutex_unlock(&que->mutex);
            return -1;
        }
    }
    
    msg = malloc(sizeof(*msg));
    if (!msg)
    {
        perror("malloc failed.");
        pthread_mutex_unlock(&que->mutex);
        return -ENOMEM;
    }
    msg->data = data;
    msg->next = NULL;

    /* 入队列 */
    if(que->que == NULL)
    {
        que->que = msg;
    }
    else
    {
        que->que_end->next = msg;
    }
    que->que_end = msg;
    que->len++;
    
    pthread_cond_signal(&que->cond_get);
    pthread_mutex_unlock(&que->mutex);

    return 0;
}

int msg_que_get(msg_que_t *que, void **data)
{
    int wait = 1;   /* 默认等待 */
    msg_t *msg;
    
    pthread_mutex_lock(&que->mutex);

    /* 队列空 */
    while (0 == que->len)
    {
        if (wait)
        {
            pthread_cond_wait(&que->cond_get, &que->mutex);
        }
        else
        {
            printf("msg que empty.\n");
            pthread_mutex_unlock(&que->mutex);
            return -1;
        }
    }
    
    /* 出队列 */
    msg = que->que;
    if(msg)
    {
        que->que = msg->next;
        if(que->que == NULL)
        {
            que->que_end = NULL;
        }
        que->len--;
        *data = msg->data;
        free(msg);
    }
    else
    {
        printf("msg que start empty.\n");
        pthread_mutex_unlock(&que->mutex);
        que->len = 0;
        return -1;
    }

    pthread_cond_signal(&que->cond_put);
    pthread_mutex_unlock(&que->mutex);

    return 0;
}
#endif

int msg_que_flush(msg_que_t *que)
{
    assert(NULL != que);
    msg_t *msg, *tmp;

    pthread_mutex_lock(&que->mutex);
    
    msg = que->que;
    while (msg)
    {
        tmp = msg->next;
        free(msg);
        msg = tmp;
    }
    
    que->que = NULL;
    que->que_end = NULL;
    que->len = 0;

    pthread_mutex_unlock(&que->mutex);
    
    return 0;
}

int msg_que_empty(msg_que_t *que)
{
    assert(NULL != que);
    int ret = 0;

    if (que->len == 0)
    {
        ret = 1;
    }

    return ret;
}

int msg_que_full(msg_que_t *que)
{
    assert(NULL != que);
    int ret = 0;

    if (que->len >= que->len_max)
    {
        ret = 1;
    }

    return ret;
}

int msg_que_full_for_type(msg_que_t *que, int type, uint32_t len_max)
{
    assert(NULL != que);
    msg_t *msg;
    uint32_t cur_len = 0;
    int ret = 0;
        
    pthread_mutex_lock(&que->mutex);
    
    msg = que->que;
    while (msg)
    {
        if (msg->type == type)
        {
            cur_len++;
        }
        msg = msg->next;
    }

    if (cur_len >= len_max)
    {
        ret = 1;
    }
    
    pthread_mutex_unlock(&que->mutex);
    
    return ret;    
}

