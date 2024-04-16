
#include "soft_g2d.h"
#include "image_g2d.h"


/* 2d硬件加速接口，在该文件中切换 */

ret_t image_fill(bitmap_t* dst, const rect_t* dst_r, color_t c) 
{
    return_value_if_fail(dst != NULL && dst_r != NULL, RET_OK);

    assert(dst_r->x >= 0 && (dst_r->x + dst_r->w) <= dst->w);
    assert(dst_r->y >= 0 && (dst_r->y + dst_r->h) <= dst->h);

    return soft_fill_rect(dst, dst_r, c);
}

ret_t image_clear(bitmap_t* dst, const rect_t* dst_r, color_t c) 
{
    return_value_if_fail(dst != NULL && dst_r != NULL, RET_OK);

    assert(dst_r->x >= 0 && (dst_r->x + dst_r->w) <= dst->w);
    assert(dst_r->y >= 0 && (dst_r->y + dst_r->h) <= dst->h);

    return soft_clear_rect(dst, dst_r, c);
}

ret_t image_copy(bitmap_t* dst, bitmap_t* src, const rect_t* src_r, xy_t dx, xy_t dy) 
{
    return_value_if_fail(dst != NULL && src != NULL && src_r != NULL, RET_OK);

    assert(dx >= 0 && (dx + src_r->w) <= dst->w);
    assert(dy >= 0 && (dy + src_r->h) <= dst->h);

    return soft_copy_image(dst, src, src_r, dx, dy);
}

ret_t image_rotate(bitmap_t* dst, bitmap_t* src, const rect_t* src_r, orientation_t o) 
{
    return_value_if_fail(dst != NULL && src != NULL && src_r != NULL, RET_OK);

    if (o == ORIENTATION_0 || o == ORIENTATION_180) 
    {
        assert(src_r->w <= dst->w);
        assert(src_r->h <= dst->h);
    } 
    else if (o == ORIENTATION_90 || o == ORIENTATION_270) 
    {
        assert(src_r->w <= dst->w);
        assert(src_r->h <= dst->h);
    }

    return soft_rotate_image(dst, src, src_r, o);
}

ret_t image_blend(bitmap_t* dst, bitmap_t* src, const rectf_t* dst_r, const rectf_t* src_r, uint8_t global_alpha) 
{
    return_value_if_fail(dst != NULL && src != NULL && dst_r != NULL && src_r != NULL, RET_BAD_PARAMS);

    assert(dst_r->x >= 0 && (dst_r->x + dst_r->w) <= dst->w);
    assert(dst_r->y >= 0 && (dst_r->y + dst_r->h) <= dst->h);

    return soft_blend_image(dst, src, dst_r, src_r, global_alpha);
}

