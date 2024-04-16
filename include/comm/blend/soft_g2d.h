
#ifndef _SOFT_G2D_H_
#define _SOFT_G2D_H_

#include "rect.h"
#include "bitmap.h"

ret_t soft_fill_rect(bitmap_t* dst, const rect_t* dst_r, color_t c);
ret_t soft_clear_rect(bitmap_t* dst, const rect_t* dst_r, color_t c);
ret_t soft_copy_image(bitmap_t* dst, bitmap_t* src, const rect_t* src_r, xy_t dx, xy_t dy);
ret_t soft_rotate_image(bitmap_t* dst, bitmap_t* src, const rect_t* src_r, orientation_t o);
ret_t soft_blend_image(bitmap_t* dst, bitmap_t* src, const rectf_t* dst_r, const rectf_t* src_r, uint8_t global_alpha);

#endif /* _SOFT_G2D_H_ */

