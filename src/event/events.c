/* 
 * 文件名称：events.c
 * 摘     要：多路事件处理
 *  
 * 修改历史   版本号         Author   修改内容
 *--------------------------------------------------
 * 2022.09.16   v1      ql     创建文件
 * 2023.03.23   v1.1    ql     添加非阻塞监听函数
 * 
 *--------------------------------------------------
 */

#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "events.h"
#include "tools.h"

#define SELECT_TIMEOUT      2000    /* in milliseconds */


void events_add_fd(struct events *events, int fd, enum event_type type, 
                       event_callback_t callback, void *priv)
{
    struct event_fd *event;

    event = malloc(sizeof(*event));
    if (event == NULL)
    {
        log_eno("malloc\n");
        return;
    }

    event->fd = fd;
    event->type = type;
    event->callback = callback;
    event->priv = priv;

    switch (event->type)
    {
        case EVENT_READ:
            FD_SET(fd, &events->rfds);
            break;
        case EVENT_WRITE:
            FD_SET(fd, &events->wfds);
            break;
        case EVENT_EXCEPTION:
            FD_SET(fd, &events->efds);
            break;
    }

    events->maxfd = max(events->maxfd, fd);

    list_append(&event->list, &events->head);
}

void events_del_fd(struct events *events, int fd, enum event_type type)
{
    struct event_fd *event = NULL;
    struct event_fd *entry;
    int maxfd = 0;

    list_for_each_entry(entry, &events->head, list)
    {
        if (entry->fd == fd && entry->type == type)
            event = entry;
        else
            maxfd = max(maxfd, entry->fd);
    }

    if (event == NULL)
        return;

    switch (event->type)
    {
        case EVENT_READ:
            FD_CLR(fd, &events->rfds);
            break;
        case EVENT_WRITE:
            FD_CLR(fd, &events->wfds);
            break;
        case EVENT_EXCEPTION:
            FD_CLR(fd, &events->efds);
            break;
    }

    events->maxfd = maxfd;

    list_remove(&event->list);
    free(event);
}

static void events_dispatch(struct events *events, const fd_set *rfds, 
                                const fd_set *wfds, const fd_set *efds)
{
    struct event_fd *event;
    struct event_fd *next;

    list_for_each_entry_safe(event, next, &events->head, list)
    {
        if (event->type == EVENT_READ &&
            FD_ISSET(event->fd, rfds))
        {
            event->callback(event);
        }
        else if (event->type == EVENT_WRITE &&
             FD_ISSET(event->fd, wfds))
        {
            event->callback(event);
        }
        else if (event->type == EVENT_EXCEPTION &&
             FD_ISSET(event->fd, efds))
        {
            event->callback(event);
        }

        /* If the callback stopped events processing, we're done. */
        if (events->done)
            break;
    }
}

int events_listen_loop(struct events *events)
{
    events->done = FALSE;

    while (!events->done)
    {
        fd_set rfds;
        fd_set wfds;
        fd_set efds;
        struct timeval timeout;
        int ret;

        rfds = events->rfds;
        wfds = events->wfds;
        efds = events->efds;

        timeout.tv_sec  = 2;
        timeout.tv_usec = 0;

        ret = select(events->maxfd + 1, &rfds, &wfds, &efds, &timeout);
        if (ret < 0)
        {
            /* EINTR means that a signal has been received, continue
             * to the next iteration in that case.
             */
            if (errno == EINTR)
            {
                continue;
            }

            log_eno("select\n");
            break;
        }
        else if (ret == 0)
        {
            // log_eno("select timeout\n");
            continue;
        }
        
        events_dispatch(events, &rfds, &wfds, &efds);
    }
    
    return RET_OK;
}


static void *events_thread_main(void *data)
{
    struct events *events = (struct events *)data;
    
    if (events)
    {
        events_listen_loop(events);
    }

    return NULL;
}

int events_listen_start(struct events *events)
{
    int ret;
    
    if (!events)
    {
        log_err("events null\n");
        return RET_FAIL;
    }

    ret = pthread_create(&events->thread_id, NULL, events_thread_main, events);
    if (ret != 0)
    {
        log_eno("pthread_create\n");
        return RET_FAIL;
    }
  
    ret = pthread_detach(events->thread_id);
    if (ret != 0)
    {
        log_eno("pthread_detach\n");
        return RET_FAIL;
    }

    return RET_OK;
}

void events_stop(struct events *events)
{
    events->done = TRUE;
}

void events_init(struct events *events)
{
    memset(events, 0, sizeof *events);

    FD_ZERO(&events->rfds);
    FD_ZERO(&events->wfds);
    FD_ZERO(&events->efds);
    events->maxfd = 0;
    list_init(&events->head);
}

void events_cleanup(struct events *events)
{
    while (!list_empty(&events->head)) {
        struct event_fd *event;

        event = list_first_entry(&events->head, typeof(*event), list);
        list_remove(&event->list);
        free(event);
    }
}

