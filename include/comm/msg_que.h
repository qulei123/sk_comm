#ifndef __MSG_QUE_H__
#define __MSG_QUE_H__

#include <stdint.h>
//#define NDEBUG            // 发布代码时, 可以打开这个宏提高效率
#include <assert.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct msg {
    struct msg *next;
    int type;               /* 消息类型 */
    void *data;
}msg_t;

typedef struct msg_que {
    msg_t *que;             /* 可以使用数组实现 */
    msg_t *que_end;
    uint32_t len;
    uint32_t len_max;
        
    pthread_mutex_t mutex;
    pthread_cond_t cond_get;
    pthread_cond_t cond_put;
}msg_que_t;


msg_que_t *msg_que_create(uint32_t len_max);
void msg_que_destroy(msg_que_t *que);
int msg_que_put(msg_que_t *que, void *data);
int msg_que_get(msg_que_t *que, void **data);
int msg_que_put_with_type(msg_que_t *que, int type, void *data);
int msg_que_get_with_type(msg_que_t *que, int *type, void **data);
int msg_que_flush(msg_que_t *que);
int msg_que_empty(msg_que_t *que);
int msg_que_full(msg_que_t *que);
int msg_que_full_for_type(msg_que_t *que, int type, uint32_t len_max);

#ifdef __cplusplus
}
#endif


#endif /* __MSG_QUE_H__ */

