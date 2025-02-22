﻿
#include "rect.h"
#include "pixel.h"
#include "bitmap.h"
#include "pixel_pack_unpack.h"

#define pixel_dst_t pixel_bgra8888_t
#define pixel_dst_format pixel_bgra8888_format
#define pixel_dst_to_rgba pixel_bgra8888_to_rgba
#define pixel_dst_from_rgb pixel_bgra8888_from_rgb
#define pixel_dst_from_rgba pixel_bgra8888_from_rgba

#define pixel_t pixel_dst_t
#define pixel_from_rgb pixel_dst_from_rgb
#define pixel_from_rgba pixel_dst_from_rgba
#define pixel_to_rgba pixel_dst_to_rgba

#define pixel_blend_rgba_dark pixel_bgra8888_blend_rgba_dark
#define pixel_blend_rgba_premulti pixel_bgra8888_blend_rgba_premulti

#include "pixel_ops.h"
#include "fill_image.h"

ret_t fill_bgra8888_rect(bitmap_t* fb, const rect_t* dst, color_t c) 
{
    return fill_image(fb, dst, c);
}


ret_t clear_bgra8888_rect(bitmap_t* fb, const rect_t* dst, color_t c) 
{
    return clear_image(fb, dst, c);
}


