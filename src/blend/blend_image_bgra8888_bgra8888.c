
#include "rect.h"
#include "pixel.h"
#include "bitmap.h"
#include "pixel_pack_unpack.h"


#define pixel_dst_t pixel_bgra8888_t
#define pixel_dst_bpp pixel_bgra8888_BPP
#define pixel_dst_format pixel_bgra8888_format
#define pixel_dst_to_rgba pixel_bgra8888_to_rgba
#define pixel_dst_from_rgb pixel_bgra8888_from_rgb
#define pixel_dst_from_rgba pixel_bgra8888_from_rgba

#define pixel_src_t pixel_bgra8888_t
#define pixel_src_format pixel_bgra8888_format
#define pixel_from_rgba pixel_dst_from_rgba
#define pixel_src_to_rgba pixel_bgra8888_to_rgba

#define pixel_t pixel_dst_t
#define pixel_from_rgb pixel_dst_from_rgb
#define pixel_to_rgba pixel_dst_to_rgba

#include "pixel_ops.h"
#include "blend_image.h"

ret_t blend_image_bgra8888_bgra8888(bitmap_t* dst, bitmap_t* src, const rectf_t* dst_r,
                                    const rectf_t* src_r, uint8_t a) 
{
    return_value_if_fail(dst != NULL && src != NULL && src_r != NULL && dst_r != NULL, RET_BAD_PARAMS);
    return_value_if_fail(dst->format == BITMAP_FMT_BGRA8888 && src->format == BITMAP_FMT_BGRA8888, RET_BAD_PARAMS);

    if (a > 0xf8) 
    {
        return blend_image_without_alpha(dst, src, dst_r, src_r);
    } 
    else if (a > 8) 
    {
        return blend_image_with_alpha(dst, src, dst_r, src_r, a);
    } 
    else 
    {
        return RET_OK;
    }
}

#if 0
ret_t blend_image_rotate_bgra8888_bgra8888(bitmap_t* dst, bitmap_t* src, const rectf_t* dst_r,
                                           const rectf_t* src_r, uint8_t a, orientation_t o) 
{
    return_value_if_fail(dst != NULL && src != NULL && src_r != NULL && dst_r != NULL, RET_BAD_PARAMS);
    return_value_if_fail(dst->format == BITMAP_FMT_BGRA8888 && src->format == BITMAP_FMT_BGRA8888, RET_BAD_PARAMS);

    if (a > 8) 
    {
        return blend_image_with_alpha_by_rotate(dst, src, dst_r, src_r, a, o);
    } 
    else 
    {
        return RET_OK;
    }
}
#endif

