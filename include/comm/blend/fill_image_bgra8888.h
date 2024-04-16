
#ifndef _FILL_IMAGE_BGRA8888_H_
#define _FILL_IMAGE_BGRA8888_H_

#include "bitmap.h"

ret_t fill_bgra8888_rect(bitmap_t* fb, const rect_t* dst, color_t c);
ret_t clear_bgra8888_rect(bitmap_t* fb, const rect_t* dst, color_t c);

#endif /* _FILL_IMAGE_BGRA8888_H_ */

