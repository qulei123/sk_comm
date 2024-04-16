
#ifndef __MSG_IPC_H__
#define __MSG_IPC_H__

#define MQ_MSG_SIZE             10
#define MQ_BUF_SIZE             512

#define MSG_IPC_PRIO_ASYNC      1
#define MSG_IPC_PRIO_SYNC       10

typedef void(*msg_ipc_handler_t)(char *buf, unsigned int size);

typedef struct resp_msg
{
    char *buf;
    unsigned int len;
} resp_msg_t;

int msg_ipc_init(msg_ipc_handler_t handler);
int msg_ipc_put(char *buf, unsigned int size); 
int msg_ipc_get(char *buf, unsigned int size);
int msg_ipc_send_sync(char *buf, unsigned int size, resp_msg_t *resp, unsigned int timeout);
int msg_ipc_resp_sync(char *buf, unsigned int size);

#endif /* __MSG_IPC_H__ */

