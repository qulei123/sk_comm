
#ifndef _PIXEL_CONFIG_H_
#define _PIXEL_CONFIG_H_

/* pixel配置 */
#define PIXEL_TYPE_RGB888
//#define PIXEL_TYPE_BGRA8888
//#define PIXEL_TYPE_RGBA8888

/** 
 * pixel格式：以各个通道在内存中存放的顺序，从低地址到高地址递增的方式命名。
 * 如：
 *    RGBA8888：表示内存地址从低到高依次为RGBA，每个通道各占一字节(8位)。
 *    BGR565：表示内存地址从低到高依次为BGR，BGR三个通道分别占5位，6位和5位。
 */
#include "pixel.h"

#ifdef PIXEL_TYPE_RGB888

#define PIXEL_FORMAT            BITMAP_FMT_RGB888
#define pixel_t                 pixel_rgb888_t
#define pixel_bpp               pixel_rgb888_BPP
#define pixel_to_rgba           pixel_rgb888_to_rgba
#define pixel_from_rgb          pixel_rgb888_from_rgb
#define pixel_from_rgba         pixel_rgb888_from_rgba

#endif	/* PIXEL_TYPE_RGB888 */


#ifdef PIXEL_TYPE_BGRA8888

#define PIXEL_FORMAT            BITMAP_FMT_BGRA8888             /* 内存地址从低到高依次为BGRA */
#define pixel_t                 pixel_bgra8888_t
#define pixel_bpp               pixel_bgra8888_BPP
#define pixel_to_rgba           pixel_bgra8888_to_rgba
#define pixel_from_rgb          pixel_bgra8888_from_rgb
#define pixel_from_rgba         pixel_bgra8888_from_rgba

#endif	/* PIXEL_TYPE_BGRA8888 */


#ifdef PIXEL_TYPE_RGBA8888

#define PIXEL_FORMAT            BITMAP_FMT_RGBA8888
#define pixel_t                 pixel_rgba8888_t
#define pixel_bpp               pixel_rgba8888_BPP
#define pixel_to_rgba           pixel_rgba8888_to_rgba
#define pixel_from_rgb          pixel_rgba8888_from_rgb
#define pixel_from_rgba         pixel_rgba8888_from_rgba

#endif	/* PIXEL_TYPE_RGBA8888 */

#endif /* _PIXEL_CONFIG_H_ */
