
#ifndef _DEF_TYPES_H_
#define _DEF_TYPES_H_

/* Includes ------------------------------------------------------------------- */
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include <errno.h>

//#define NDEBUG            // 发布代码时, 可以打开这个宏提高效率
#include <assert.h>

/**
 * 函数返回值常量定义。
 */
typedef enum _ret_t 
{
    RET_OK = 0,         /* 成功 */
    RET_FAIL,           /* 失败 */
    RET_OOM,            /* Out of memory */
    RET_NOT_IMPL,       /* 没有实现/不支持 */
    RET_QUIT,           /* 退出。通常用于主循环 */
    RET_FOUND,          /* 找到 */
    RET_BUSY,           /* 对象忙 */
    RET_REMOVE,         /* 移出。通常用于定时器 */
    RET_REPEAT,         /* 重复。通常用于定时器 */
    RET_NOT_FOUND,      /* 没找到 */
    RET_DONE,           /* 操作完成 */
    RET_STOP,           /* 停止后续操作 */
    RET_SKIP,           /* 跳过当前项 */
    RET_CONTINUE,       /* 继续后续操作 */
    RET_OBJECT_CHANGED, /* 对象属性变化 */
    RET_ITEMS_CHANGED,  /* 集合数目变化 */
    RET_BAD_PARAMS,     /* 无效参数 */
    RET_TIMEOUT,        /* 超时 */
    RET_CRC,            /* CRC错误 */
    RET_IO,             /* IO错误 */
    RET_EOS,            /* End of Stream */
    RET_NOT_MODIFIED,   /* 没有改变 */
} ret_t;

#define FD_INVALID  -1

/* 基本类型定义 */
typedef void        VOID;
typedef char        CHAR;
typedef int         INT;
typedef float       FLOAT;
typedef double      DOUBLE;

typedef uint64_t    U64;
typedef uint32_t    U32;
typedef uint16_t    U16;
typedef uint8_t     U8;

typedef int64_t     S64;
typedef int32_t     S32;
typedef int16_t     S16;
typedef int8_t      S8;

typedef U16         BE16;
typedef U32         BE32;

/**
 * @brief Boolean Type definition
 */
typedef enum {FALSE = 0, TRUE = !FALSE} BOOL;
typedef BOOL bool_t;


/**
 * Read/Write transfer type mode (Block or non-block)
 */
typedef enum
{
    NONE_BLOCKING = 0,      /**< None Blocking type */
    BLOCKING                /**< Blocking type */
} E_BLOCK_Type;


#if 0
#if defined(__GNUC__) && !defined(__cplusplus)
typedef _Bool bool_t;
#else
typedef uint8_t bool_t;
#endif


typedef int32_t xy_t;
typedef int32_t wh_t;
typedef uint16_t font_size_t;
#endif

/**
 * @enum orientation_t
 * 旋转角度。
 */
typedef enum _orientation_t 
{
    ORIENTATION_0 = 0,                  /* 没有旋转。 */
    ORIENTATION_90 = 90,                /* 旋转90度。 */
    ORIENTATION_180 = 180,              /* 旋转180度。 */
    ORIENTATION_270 = 270               /* 旋转270度。 */
} orientation_t;


/* zlog 宏定义 */
#ifdef ZLOG_PRINT
#include "zlog.h"

#define  log_hex                hdzlog_info
#define  log_info               dzlog_info
#define  log_dbg                dzlog_debug
#define  log_war                dzlog_warn
#define  log_err                dzlog_error
//#define  log_eno(fmt,...)     dzlog_error(fmt"errno:%d\n", ##__VA_ARGS__, errno)
#define  log_eno(fmt, args...) \
    dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
    ZLOG_LEVEL_ERROR, "[errno:%d] "fmt, errno, ##args)

#else

#define  log_hex(buf, len)           \
do                                  \
{                                   \
    for (int i = 0; i < len; i++)   \
    {                               \
        printf("%02X ", (U8)buf[i]);    \
    }                               \
    printf("\n");                   \
}                                   \
while (0)

/* C++11要求，当字符串跟变量连接的时候，必须fmt前后增加一个空格才行。 */
#define  log_info(fmt, ...)      printf( fmt , ##__VA_ARGS__)
#define  log_dbg(fmt, ...)       printf( fmt , ##__VA_ARGS__)
#define  log_war(fmt, ...)       printf( fmt , ##__VA_ARGS__)
#define  log_err(fmt, ...)       printf("%s[%d]-" fmt , __FILE__, __LINE__, ##__VA_ARGS__)
#define  log_eno(fmt, ...)       printf("%s[%d]-eno[%d]" fmt, __FILE__, __LINE__, errno, ##__VA_ARGS__)

#endif

#if defined(NDEBUG)

#define ENSURE(p) (void)(p)
#define goto_error_if_fail(p) \
  if (!(p)) {                 \
    goto error;               \
  }

#define return_if_fail(p) \
  if (!(p)) {             \
    return;               \
  }

#define break_if_fail(p) \
  if (!(p)) {            \
    break;               \
  }

#define return_value_if_fail(p, value) \
  if (!(p)) {                          \
    return (value);                    \
  }
  
#else

#define ENSURE(p) assert(p)
#define goto_error_if_fail(p)                                              \
  if (!(p)) {                                                              \
    log_info("%s:%d condition(" #p ") failed!\n", __FUNCTION__, __LINE__); \
    goto error;                                                            \
  }

#define break_if_fail(p)                                                   \
  if (!(p)) {                                                              \
    log_info("%s:%d condition(" #p ") failed!\n", __FUNCTION__, __LINE__); \
    break;                                                                 \
  }

#define return_if_fail(p)                                                  \
  if (!(p)) {                                                              \
    log_info("%s:%d condition(" #p ") failed!\n", __FUNCTION__, __LINE__); \
    return;                                                                \
  }

#define return_value_if_fail(p, value)                                     \
  if (!(p)) {                                                              \
    log_info("%s:%d condition(" #p ") failed!\n", __FUNCTION__, __LINE__); \
    return (value);                                                        \
  }

#endif

#endif   /* _DEF_TYPES_H_ */

