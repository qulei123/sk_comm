
#include "rect.h"
#include "pixel.h"
#include "bitmap.h"
#include "pixel_pack_unpack.h"

#define pixel_dst_t pixel_bgra8888_t
#define pixel_dst_format pixel_bgra8888_format
#define pixel_dst_to_rgba pixel_bgra8888_to_rgba
#define pixel_dst_from_rgb pixel_bgra8888_from_rgb

#include "rotate_image.h"

ret_t rotate_bgra8888_image(bitmap_t* fb, bitmap_t* img, const rect_t* src, orientation_t o) 
{
    return rotate_image(fb, img, src, o);
}

ret_t rotate_bgra8888_image_ex(bitmap_t* fb, bitmap_t* img, const rect_t* src, xy_t dx, xy_t dy, orientation_t o) 
{
    return rotate_image_ex(fb, img, src, dx, dy, o);
}

