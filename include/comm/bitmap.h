
#ifndef _BITMAP_H_
#define _BITMAP_H_

#include "color.h"
#include "deftypes.h"


#ifdef __cplusplus
extern "C" {
#endif


#ifndef BITMAP_ALIGN_SIZE
#define BITMAP_ALIGN_SIZE 32
#endif /*BITMAP_ALIGN_SIZE*/


typedef enum _bitmap_format_t
{
  /**
   * @const BITMAP_FMT_NONE
   * 无效格式。
   */
  BITMAP_FMT_NONE = 0,
  /**
   * @const BITMAP_FMT_RGBA8888
   * 一个像素占用4个字节，RGBA占一个字节，按内存地址递增。
   */
  BITMAP_FMT_RGBA8888,
  /**
   * @const BITMAP_FMT_ABGR8888
   * 一个像素占用4个字节，ABGR占一个字节，按内存地址递增。
   */
  BITMAP_FMT_ABGR8888,
  /**
   * @const BITMAP_FMT_BGRA8888
   * 一个像素占用4个字节，BGRA占一个字节，按内存地址递增。
   */
  BITMAP_FMT_BGRA8888,
  /**
   * @const BITMAP_FMT_ARGB8888
   * 一个像素占用4个字节，ARGB占一个字节，按内存地址递增。
   */
  BITMAP_FMT_ARGB8888,
  /**
   * @const BITMAP_FMT_RGB565
   * 一个像素占用2个字节，RGB分别占用5,6,5位, 按内存地址递增。
   */
  BITMAP_FMT_RGB565,
  /**
   * @const BITMAP_FMT_BGR565
   * 一个像素占用2个字节，BGR分别占用5,6,5位, 按内存地址递增。
   */
  BITMAP_FMT_BGR565,
  /**
   * @const BITMAP_FMT_RGB888
   * 一个像素占用3个字节，RGB占一个字节，按内存地址递增。
   */
  BITMAP_FMT_RGB888,
  /**
   * @const BITMAP_FMT_BGR888
   * 一个像素占用3个字节，RGB占一个字节，按内存地址递增。
   */
  BITMAP_FMT_BGR888,
  /**
   * @const BITMAP_FMT_GRAY
   * 一个像素占用1个字节。
   */
  BITMAP_FMT_GRAY,
  /**
   * @const BITMAP_FMT_MONO
   * 一个像素占用1比特。
   */
  BITMAP_FMT_MONO,
  /**
   * @const BITMAP_FMT_BGRA5551
   * 一个像素占用2个字节，BGRA分别占用5,6,5,1位, 按内存地址递增。
   */
  BITMAP_FMT_BGRA5551,
} bitmap_format_t;


/**
 * @enum bitmap_flag_t
 * @annotation ["scriptable"]
 * @prefix BITMAP_FLAG_
 * 位图标志常量定义。
 */
typedef enum _bitmap_flag_t 
{
    BITMAP_FLAG_NONE = 0,                   /* 无特殊标志 */
    BITMAP_FLAG_OPAQUE = 1,                 /* 不透明图片 */
    BITMAP_FLAG_IMMUTABLE = 1 << 1,         /* 图片内容不会变化 */
    BITMAP_FLAG_TEXTURE = 1 << 2,           /* OpenGL Texture, bitmap的id是有效的texture id */
    BITMAP_FLAG_CHANGED = 1 << 3,           /* 如果是MUTABLE的图片，更新时需要设置此标志，底层可能会做特殊处理，比如更新图片到GPU */
    BITMAP_FLAG_PREMULTI_ALPHA = 1 << 4,    /* 预乘alpha。 */
    BITMAP_FLAG_LCD_ORIENTATION = 1 << 5,   /* 位图数据已经处理了 lcd 旋转，同时说明 bitmap 的宽高和真实数据的宽高可能不一致 */
    BITMAP_FLAG_GPU_FBO_TEXTURE = 1 << 6,   /* 该位图为 GPU 的 fbo 数据 */

    BITMAP_FLAG_USED = 1 << 15,             /* 该位图正在使用 */
} bitmap_flag_t;


/**
 * @class bitmap_t
 * @order -9
 * @annotation ["scriptable"]
 * 位图。
 */
typedef struct _bitmap_t 
{
    int w;                         /* 宽度 */
    int h;                         /* 高度 */
    uint32_t line_length;           /* 每一行实际占用的内存(也称为stride或pitch)，一般情况下为w*bpp */
    uint32_t bpp;                   /* byte per pixel, 1个像素点占多少字节 */
    uint16_t flags;                 /* 标志。请参考{bitmap_flag_t} */     
    uint16_t format;                /* 格式。请参考{bitmap_format_t} */
    const char* name;               /* 名字 */
    uint8_t* buffer;                /* 图片数据 */
    uint32_t size;                  /* buffer大小 */
    char shm_name[16];              /* 共享内存节点的名字 */
    uint32_t r;                     /* 是否需要将图片裁剪为圆形；0，表示不裁剪 */
    int ref_count;                  /* 引用计数, 原子操作, 修改时需要加锁 */

    bool_t should_free_handle;      /* destroy时是否需要释放bitmap本身的内存 */
    bool_t should_free_data;        /* destroy时是否需要释放data指向的内存 */
}bitmap_t;


/* bitmap buf 集合，用于队列发送数据 */
typedef struct _bitmap_buf_set_t
{
    bitmap_t *bufs;
    uint32_t nbufs;
    uint32_t next;       /* 下一个索引号 */
}bitmap_buf_set_t;


/**
 * @method bitmap_create
 * 创建图片对象(一般供脚本语言中使用)。
 * @annotation ["constructor", "scriptable", "gc"]
 * @return {bitmap_t*} 返回bitmap对象。
 */
bitmap_t* bitmap_create(void);


/**
 * @method bitmap_get_size
 * 获取一张图片占用的字节数。
 * @annotation ["scriptable"]
 * @param {uint32_t} w 宽度。
 * @param {uint32_t} h 高度。
 * @param {bitmap_format_t} format 格式。
 *
 * @return {uint32_t} 返回一张图片占用的字节数。
 */
uint32_t bitmap_get_size(uint32_t w, uint32_t h, bitmap_format_t format);

/**
 * @method bitmap_get_bpp
 * 获取图片一个像素占用的字节数。
 * @annotation ["scriptable"]
 * @param {bitmap_t*} bitmap bitmap对象。
 *
 * @return {uint32_t} 返回一个像素占用的字节数。
 */
uint32_t bitmap_get_bpp(bitmap_t* bitmap);

/**
 * @method bitmap_set_line_length
 * 设置line_length。
 * @param {bitmap_t*} bitmap bitmap对象。
 * @param {uint32_t} line_length line_length。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t bitmap_set_line_length(bitmap_t* bitmap, uint32_t line_length);

/**
 * @method bitmap_get_line_length
 * 获取每一行占用内存的字节数。
 * @param {bitmap_t*} bitmap bitmap对象。
 *
 * @return {uint32_t} 返回每一行占用内存的字节数。
 */
uint32_t bitmap_get_line_length(bitmap_t* bitmap);

/**
 * @method bitmap_get_width
 * 获取图片的宽度。
 * @param {bitmap_t*} bitmap bitmap对象。
 *
 * @return {uint32_t} 返回图片宽度。
 */
uint32_t bitmap_get_width(bitmap_t* bitmap);

/**
 * @method bitmap_get_height
 * 获取图片高度。
 * @param {bitmap_t*} bitmap bitmap对象。
 *
 * @return {uint32_t} 返回图片高度。
 */
uint32_t bitmap_get_height(bitmap_t* bitmap);

/**
 * @method bitmap_get_mem_size
 * 获取图片高度。
 * @param {bitmap_t*} bitmap bitmap对象。
 *
 * @return {uint32_t} 返回图片高度。
 */
uint32_t bitmap_get_mem_size(bitmap_t* bitmap);

void bitmap_printf(bitmap_t* bitmap);

/**
 * @method bitmap_clone
 * Clone图片。
 * @param {bitmap_t*} bitmap bitmap对象。
 *
 * @return {bitmap_t*} 返回新的bitmap对象。
 */
bitmap_t* bitmap_clone(bitmap_t* bitmap);

/**
 * @method bitmap_init
 * 初始化图片。
 * @param {bitmap_t*} bitmap bitmap对象。
 * @param {uint32_t} w 宽度。
 * @param {uint32_t} h 高度。
 * @param {bitmap_format_t} format 格式。
 * @param {const uint8_t*} data 数据，直接引用，但不负责释放。如果为空，由内部自动分配和释放。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t bitmap_init(bitmap_t* bitmap, uint32_t w, uint32_t h, bitmap_format_t format, uint8_t* data);

/**
 * @method bitmap_save_png
 * 把bitmap保存为png。
 *
 * @param {bitmap_t*} bitmap bitmap对象。
 * @param {const char*} filename 文件名。
 *
 * @return {bool_t} 返回TRUE表示成功，FALSE表示失败。
 */
bool_t bitmap_save_png(bitmap_t* bitmap, const char* filename);

/**
 * @method bitmap_destroy_with_self
 * 销毁图片(for script only)。
 * @alias bitmap_destroy
 * @annotation ["deconstructor", "scriptable", "gc"]
 * @param {bitmap_t*} bitmap bitmap对象。
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t bitmap_destroy_with_self(bitmap_t* bitmap);

/**
 * @method bitmap_destroy
 * 销毁图片。
 * @annotation ["deconstructor"]
 * @param {bitmap_t*} bitmap bitmap对象。
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t bitmap_destroy(bitmap_t* bitmap);

/**
 * @method bitmap_get_bpp_of_format
 * 获取位图格式对应的颜色位数。
 * @annotation ["scriptable", "static"]
 * 
 * @param {bitmap_format_t} format 位图格式。
 * 
 * @return {uint32_t} 成功返回颜色位数，失败返回0。
 */
uint32_t bitmap_get_bpp_of_format(bitmap_format_t format);

/*private*/
ret_t bitmap_alloc_data(bitmap_t* bitmap);


/* 队列使用 */
bitmap_buf_set_t *bitmap_buf_set_create(uint32_t w, uint32_t h, bitmap_format_t format, uint32_t nbufs);
void bitmap_buf_set_destroy(bitmap_buf_set_t *buf_set);
bitmap_t *bitmap_buf_obtain(bitmap_buf_set_t *buf_set);
void bitmap_buf_release(bitmap_t *bitmap);
void bitmap_buf_ref(bitmap_t *bitmap);
void bitmap_buf_unref(bitmap_t *bitmap);


#ifdef __cplusplus
}
#endif

#endif /*_BITMAP_H_*/

