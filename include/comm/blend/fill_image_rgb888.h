
#ifndef _FILL_IMAGE_RGB888_H_
#define _FILL_IMAGE_RGB888_H_

#include "bitmap.h"

ret_t fill_rgb888_rect(bitmap_t* fb, const rect_t* dst, color_t c);
ret_t clear_rgb888_rect(bitmap_t* fb, const rect_t* dst, color_t c);

#endif  /* _FILL_IMAGE_RGB888_H_ */

