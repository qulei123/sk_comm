
#ifndef _BLEND_IMAGE_RGB888_RGBA8888_H_
#define _BLEND_IMAGE_RGB888_RGBA8888_H_

#include "bitmap.h"

ret_t blend_image_rgb888_rgba8888(bitmap_t* dst, bitmap_t* src, const rectf_t* dst_r,
                                  const rectf_t* src_r, uint8_t a);

ret_t blend_image_rotate_rgb888_rgba8888(bitmap_t* dst, bitmap_t* src, const rectf_t* dst_r,
                                         const rectf_t* src_r, uint8_t a, orientation_t o);

#endif /* _BLEND_IMAGE_RGB888_RGBA8888_H_ */

