
#ifndef _FILL_IMAGE_RGBA8888_H_
#define _FILL_IMAGE_RGBA8888_H_

#include "bitmap.h"

ret_t fill_rgba8888_rect(bitmap_t* fb, const rect_t* dst, color_t c);
ret_t clear_rgba8888_rect(bitmap_t* fb, const rect_t* dst, color_t c);

#endif /* _FILL_IMAGE_RGBA8888_H */

