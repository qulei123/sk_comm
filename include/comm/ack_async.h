
#ifndef _ACK_ASYNC_H_
#define _ACK_ASYNC_H_


/* 重发次数最大值，可以认为是一直重发 */
#define RE_NUM_MAX      (uint32_t)(0xFFFFFFFF)

typedef struct ack_info
{
    int cmd;
    uint32_t msg_id;
    int re_time;        /* 重发间隔时间，单位秒；与scan_time配合使用 */
    uint32_t re_num;    /* 重发次数 */
    int time_cnt;       /* 时间计数 */
} ack_info_t;

typedef struct ack_cb
{
    struct list_entry list;
    ack_info_t ack;
    int payload_len;    /* resend 数据的长度 */
    uint8_t *payload;
} ack_cb_t;

typedef int (*resend_t)(ack_cb_t *ack_cb);

/* 响应处理上下文 */
typedef struct ack_ctx
{
    struct list_entry ack_list;
    resend_t resend;
    int scan_time;       /* 遍历链表的间隔时间，单位秒；0表示停止 */
} ack_ctx_t;


ack_ctx_t *ack_async_init(resend_t resend);
void ack_async_exit(ack_ctx_t *ctx);
int ack_list_record(ack_ctx_t *ctx, ack_info_t *ack, uint8_t *buf, int buf_len);
int ack_list_unrecord(ack_ctx_t *ctx, int cmd, uint32_t msg_id);
void ack_list_print(ack_ctx_t *ctx);


#endif  /* _ACK_ASYNC_H_ */

