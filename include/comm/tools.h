
#ifndef __TOOLS_H__
#define __TOOLS_H__

#include "deftypes.h"

/* Number of elements in an array */
#define ARRAY_SIZE(array)   (sizeof(array) / sizeof((array)[0]))

#define min(a, b) ({                \
    typeof(a) __a = (a);            \
    typeof(b) __b = (b);            \
    __a < __b ? __a : __b;          \
})

#define min_t(type, a, b) ({        \
    type __a = (a);                 \
    type __b = (b);                 \
    __a < __b ? __a : __b;          \
})

#define max(a, b) ({                \
    typeof(a) __a = (a);            \
    typeof(b) __b = (b);            \
    __a > __b ? __a : __b;          \
})

#define max_t(type, a, b) ({        \
    type __a = (a);                 \
    type __b = (b);                 \
    __a > __b ? __a : __b;          \
})

#define clamp(val, min, max) ({     \
    typeof(val) __val = (val);      \
    typeof(min) __min = (min);      \
    typeof(max) __max = (max);      \
    __val = __val < __min ? __min : __val;  \
    __val > __max ? __max : __val;  \
})

#define clamp_t(type, val, min, max) ({     \
    type __val = (val);             \
    type __min = (min);             \
    type __max = (max);             \
    __val = __val < __min ? __min : __val;  \
    __val > __max ? __max : __val;  \
})

#define ROUND_TO(size, round_size)  ((((size) + (round_size)-1) / (round_size)) * (round_size))
#define div_round_up(num, denom)    (((num) + (denom) - 1) / (denom))
#define roundi(a)                   (int32_t)(((a) >= 0) ? ((a) + 0.5f) : ((a)-0.5f))

#define container_of(ptr, type, member) \
    (type *)((char *)(ptr) - offsetof(type, member))

#if !defined(MAX)
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#if !defined(MIN)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/* 64位数据类型有bug */
#define ABS(x)                  \
({                              \
    int32_t __x = (x);          \
    (__x < 0) ? -__x : __x;     \
})

#define fix_xywh(x, y, w, h) \
  if (w < 0) {               \
    w = -w;                  \
    x = x - w + 1;           \
  }                          \
  if (h < 0) {               \
    h = -h;                  \
    y = y - h + 1;           \
  }


/********************** 获得该地址上的数据 **********************/
#define  get_mem_8b(x)      (*((uint8_t *)(x)))
#define  get_mem_16b(x)     (*((uint16_t *)(x)))
#define  get_mem_32b(x)     (*((uint32_t *)(x)))

/***************** 将主机数据类型转化为大端模式（字符串模式） *******************/
#define host_to_be16(a)     ((((uint16_t) (a) << 8) & 0xff00) | (((uint16_t) (a) >> 8) & 0xff))

#define host_to_be32(a)     ((((uint32_t) (a) << 24) & 0xff000000) |        \
                            (((uint32_t) (a) << 8) & 0xff0000)  |            \
                            (((uint32_t) (a) >> 8) & 0xff00)  |              \
                            (((uint32_t) (a) >> 24) & 0xff))

#define be16_to_host   host_to_be16
#define be32_to_host   host_to_be32


int sys_get_eth0_ip(char *buf, int len);
char *strlcpy(char *dest, const char *src, size_t n);
uint16_t* memset16(uint16_t* buff, uint16_t val, uint32_t size);
uint32_t* memset24(uint32_t* buff, void* val, uint32_t size);
uint32_t* memset32(uint32_t* buff, uint32_t val, uint32_t size);

typedef int (*compare_t)(const void* a, const void* b);
ret_t quick_sort(void** array, size_t nr, compare_t cmp);
int32_t pointer_to_int(const void* p);
void* pixel_copy(void* dst, const void* src, uint32_t size, uint8_t bpp);

#endif /* __TOOLS_H__ */

