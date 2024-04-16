
#include "tools.h"
#include "pixel.h"
#include "soft_g2d.h"
#include "pixel_pack_unpack.h"

#include "fill_image_bgra8888.h"
#include "fill_image_rgba8888.h"
#include "fill_image_rgb888.h"

#include "blend_image_rgba8888_bgra8888.h"
#include "blend_image_rgba8888_rgba8888.h"
#include "blend_image_bgra8888_bgra8888.h"
#include "blend_image_bgra8888_rgba8888.h"
#include "blend_image_bgra8888_rgb888.h"
#include "blend_image_rgb888_bgra8888.h"
#include "blend_image_rgb888_rgba8888.h"
#include "blend_image_bgr888_bgra8888.h"
#include "blend_image_bgr888_rgba8888.h"

#include "rotate_image_bgra8888.h"
#include "rotate_image_rgba8888.h"

ret_t soft_copy_image(bitmap_t* dst, bitmap_t* src, const rect_t* src_r, xy_t dx, xy_t dy) 
{
    uint8_t* src_p = NULL;
    uint8_t* dst_p = NULL;
    uint8_t* src_data = NULL;
    uint8_t* dst_data = NULL;
    
    uint32_t bpp = bitmap_get_bpp(dst);
    uint32_t dst_w = dst->w;
    uint32_t dst_h = dst->h;
    uint32_t src_w = src->w;
    uint32_t src_h = src->h;
    uint32_t dst_line_length = dst->line_length;
    uint32_t src_line_length = src->line_length;
    return_value_if_fail(dst != NULL && src != NULL && src_r != NULL, RET_BAD_PARAMS);
    return_value_if_fail(dst->format == src->format, RET_BAD_PARAMS);

    src_data = src->buffer;
    dst_data = dst->buffer;
    return_value_if_fail(src_data != NULL && dst_data != NULL, RET_BAD_PARAMS);

    src_p = (uint8_t*)(src_data) + src_r->y * src_line_length + src_r->x * bpp;
    dst_p = (uint8_t*)(dst_data) + dy * dst_line_length + dx * bpp;
    
    if ((dst_w * bpp == dst_line_length) && (src_w * bpp == src_line_length) && dst_w == src_w &&
      dst_h == src_h && src_r->w == src_w && src_r->x == 0) 
    {
        uint32_t size = (src_r->w * src_r->h);
        pixel_copy(dst_p, src_p, size, bpp);
    } 
    else 
    {
        uint32_t i = 0;
        uint32_t size = src_r->w;

        for (i = 0; i < src_r->h; i++)
        {
            pixel_copy(dst_p, src_p, size, bpp);
            dst_p += dst_line_length;
            src_p += src_line_length;
        }
    }

    return RET_OK;
}

ret_t soft_clear_rect(bitmap_t* dst, const rect_t* dst_r, color_t c) 
{
    return_value_if_fail(dst != NULL && dst_r != NULL, RET_BAD_PARAMS);

    switch (dst->format) 
    {
        case BITMAP_FMT_RGBA8888: 
        {
            return clear_rgba8888_rect(dst, dst_r, c);
        }
        case BITMAP_FMT_BGRA8888: 
        {
            return clear_bgra8888_rect(dst, dst_r, c);
        }
        case BITMAP_FMT_RGB888:
        {
            return clear_rgb888_rect(dst, dst_r, c);
        }
        default:
            break;
    }

    assert(!"not supported format");

    return RET_NOT_IMPL;
}

ret_t soft_fill_rect(bitmap_t* dst, const rect_t* dst_r, color_t c) 
{
    return_value_if_fail(dst != NULL && dst_r != NULL, RET_BAD_PARAMS);

    switch (dst->format) 
    {
        case BITMAP_FMT_RGBA8888: 
        {
            return fill_rgba8888_rect(dst, dst_r, c);
        }
        case BITMAP_FMT_BGRA8888: 
        {
            return fill_bgra8888_rect(dst, dst_r, c);
        }
        case BITMAP_FMT_RGB888:
        {
            return fill_rgb888_rect(dst, dst_r, c);
        }
        default:
            break;
    }

    assert(!"not supported format");

    return RET_NOT_IMPL;
}

ret_t soft_blend_image(bitmap_t* dst, bitmap_t* src, const rectf_t* dst_r, const rectf_t* src_r, uint8_t alpha) 
{
    return_value_if_fail(dst != NULL && src != NULL && src_r != NULL && dst_r != NULL, RET_BAD_PARAMS);

    switch (dst->format) 
    {
        case BITMAP_FMT_BGRA8888: 
        {
            switch (src->format) 
            {
                case BITMAP_FMT_RGBA8888: 
                {
                    return blend_image_bgra8888_rgba8888(dst, src, dst_r, src_r, alpha);
                }
                case BITMAP_FMT_BGRA8888: 
                {
                    return blend_image_bgra8888_bgra8888(dst, src, dst_r, src_r, alpha);
                }
                case BITMAP_FMT_RGB888:
                {
                    return blend_image_bgra8888_rgb888(dst, src, dst_r, src_r, alpha);
                }
                default:
                    break;
            }
            break;
        }
        case BITMAP_FMT_RGBA8888: 
        {
            switch (src->format) 
            {
                case BITMAP_FMT_RGBA8888: 
                {
                    return blend_image_rgba8888_rgba8888(dst, src, dst_r, src_r, alpha);
                }
                case BITMAP_FMT_BGRA8888: 
                {
                    return blend_image_rgba8888_bgra8888(dst, src, dst_r, src_r, alpha);
                }
                default:
                    break;
            }
            break;
        }
        case BITMAP_FMT_BGR888:
        {
            switch (src->format) 
            {
                case BITMAP_FMT_RGBA8888: 
                {
                    return blend_image_bgr888_rgba8888(dst, src, dst_r, src_r, alpha);
                }
                case BITMAP_FMT_BGRA8888: 
                {
                    return blend_image_bgr888_bgra8888(dst, src, dst_r, src_r, alpha);
                }
                default:
                    break;
            }
            break;
        }
        case BITMAP_FMT_RGB888:
        {
            switch (src->format) 
            {
                case BITMAP_FMT_RGBA8888: 
                {
                    return blend_image_rgb888_rgba8888(dst, src, dst_r, src_r, alpha);
                }
                case BITMAP_FMT_BGRA8888: 
                {
                    return blend_image_rgb888_bgra8888(dst, src, dst_r, src_r, alpha);
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
  
    assert(!"not supported format");
    
    return RET_NOT_IMPL;
}


ret_t soft_rotate_image(bitmap_t* dst, bitmap_t* src, const rect_t* src_r, orientation_t o) 
{
    return_value_if_fail(dst != NULL && src != NULL && src_r != NULL, RET_BAD_PARAMS);
    return_value_if_fail(dst->format == src->format, RET_BAD_PARAMS);

    switch (dst->format) 
    {
        case BITMAP_FMT_RGBA8888: 
        {
            return rotate_rgba8888_image(dst, src, src_r, o);
        }
        case BITMAP_FMT_BGRA8888: 
        {
            return rotate_bgra8888_image(dst, src, src_r, o);
        }
        #if 0
        case BITMAP_FMT_BGR888: 
        {
            return rotate_bgr888_image(dst, src, src_r, o);
        }
        case BITMAP_FMT_RGB888: 
        {
            return rotate_rgb888_image(dst, src, src_r, o);
        }
        #endif    
        default:
            break;
    }

    assert(!"not supported format");

    return RET_NOT_IMPL;
}

