
#ifndef __EVENTS_H__
#define __EVENTS_H__

#include "list.h"

struct event_fd;

typedef void (*event_callback_t)(struct event_fd *event);

struct events
{
    struct list_entry head;
    int done;
    pthread_t thread_id;

    int maxfd;
    fd_set rfds;
    fd_set wfds;
    fd_set efds;
};

enum event_type 
{
    EVENT_READ = 1,
    EVENT_WRITE = 2,
    EVENT_EXCEPTION = 4,
};

struct event_fd
{
    struct list_entry list;

    int fd;
    enum event_type type;
    event_callback_t callback;
    void *priv;
};

void events_add_fd(struct events *events, int fd, enum event_type type, event_callback_t callback, void *priv);
void events_del_fd(struct events *events, int fd, enum event_type type);

int events_listen_loop(struct events *events);
int events_listen_start(struct events *events);
void events_stop(struct events *events);

void events_init(struct events *events);
void events_cleanup(struct events *events);

#endif /* __EVENTS_H__ */

