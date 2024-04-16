/**
 * File:   fill_image_rgb888.c
 * Author: ql
 * Brief:  fill on rgb888
 *
 * History:
 * ================================================================
 * 2023-06-08 Generated
 *
 */
#include "rect.h"
#include "pixel.h"
#include "bitmap.h"
#include "pixel_pack_unpack.h"

#define pixel_dst_t pixel_rgb888_t
#define pixel_dst_format pixel_rgb888_format
#define pixel_dst_to_rgba pixel_rgb888_to_rgba
#define pixel_dst_from_rgb pixel_rgb888_from_rgb
#define pixel_dst_from_rgba pixel_rgb888_from_rgba

#define pixel_t pixel_dst_t
#define pixel_from_rgb pixel_dst_from_rgb
#define pixel_from_rgba pixel_dst_from_rgba
#define pixel_to_rgba pixel_dst_to_rgba

#define pixel_blend_rgba_dark pixel_rgb888_blend_rgba_dark
#define pixel_blend_rgba_premulti pixel_rgb888_blend_rgba_premulti

#include "pixel_ops.h"
#include "fill_image.h"

ret_t fill_rgb888_rect(bitmap_t* fb, const rect_t* dst, color_t c)
{
    return fill_image(fb, dst, c);
}

ret_t clear_rgb888_rect(bitmap_t* fb, const rect_t* dst, color_t c)
{
    return clear_image(fb, dst, c);
}

