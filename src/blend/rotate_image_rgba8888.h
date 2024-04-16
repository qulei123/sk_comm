
#ifndef _ROTATE_IMAGE_RGBA8888_H_
#define _ROTATE_IMAGE_RGBA8888_H_

#include "bitmap.h"

ret_t rotate_rgba8888_image(bitmap_t* fb, bitmap_t* img, const rect_t* src, orientation_t o);
ret_t rotate_rgba8888_image_ex(bitmap_t* fb, bitmap_t* img, const rect_t* src, xy_t dx, xy_t dy, orientation_t o);

#endif /* _ROTATE_IMAGE_RGBA8888_H */

