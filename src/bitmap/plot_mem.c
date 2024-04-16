
#include <string.h>
#include "tools.h"
#include "pixel.h"
#include "image_g2d.h"
#include "plot_mem.h"

/* pixel_config.h需要放到  pixel_ops.h之前 */
#include "pixel_config.h"
#include "pixel_ops.h"

/* 全局透明度 */
uint8_t g_alpha = 0xFF;

/* pixel_t 需要定义到配置文件中, 说明像素点的格式 */

ret_t set_global_alpha(uint8_t alpha)
{
    g_alpha = alpha;
    return RET_OK;
}

#if 0
ret_t mem_draw_glyph8(bitmap_t* fb, PT_Glyph glyph, const rect_t *src, xy_t x, xy_t y, color_t color)
{
    wh_t i = 0;
    wh_t j = 0;
    wh_t d_offset = sizeof(pixel_t);
    pixel_t* dst_p = NULL;
    uint32_t glyph_w = glyph->w;
    uint8_t color_alpha = (color.rgba.a * g_alpha) >> 8;
    uint32_t line_length = fb->line_length;
    uint8_t* fbuff = fb->buffer;
    const uint8_t* src_p = glyph->data + glyph->w * src->y + src->x;        /* 字形起始位置 */
    wh_t dst_offset = line_length;
    pixel_t pixel = color_to_pixel(color);

    dst_p = (pixel_t*)(fbuff + (y + j) * line_length) + x;

    for (j = 0; j < src->h; j++) 
    {
        const uint8_t* s = src_p;
        pixel_t* d = dst_p;

        for (i = 0; i < src->w; i++, s++) 
        {
            uint8_t a = (*s * color_alpha) >> 8;
            if (a >= ALPHA_OPACITY) 
            {
                *d = pixel;
            } 
            else if (a >= ALPHA_TRANSPARENT) 
            {
                color.rgba.a = a;
                *d = blend_pixel(*d, color);
            }

            /* 指向下一个位置 */
            d = (pixel_t*)(((uint8_t*)d) + d_offset);
        }
        //log_info("\n");
        src_p += glyph_w;
        dst_p = (pixel_t*)(((uint8_t*)dst_p) + dst_offset);
    }

    return RET_OK;
}
#endif

ret_t mem_draw_points(bitmap_t* fb, point_t* points, uint32_t nr, color_t color)
{
    wh_t i = 0;
    pixel_t pixel = color_to_pixel(color);
    uint8_t a = (color.rgba.a * g_alpha) / 0xff;
    uint32_t line_length = fb->line_length;
    uint8_t* fbuff = fb->buffer;

    for (i = 0; i < nr; i++) 
    {
        point_t* point = points + i;
        xy_t x = point->x;
        xy_t y = point->y;
        pixel_t* p = (pixel_t*)(fbuff + y * line_length) + x;

        if (a >= ALPHA_OPACITY) 
        {
            *p = pixel;
        } 
        else if (a >= ALPHA_TRANSPARENT) 
        {
            *p = blend_pixel(*p, color);
        }
    }

    return RET_OK;
}

ret_t mem_draw_image(bitmap_t *fb, bitmap_t *img, const rectf_t* src, const rectf_t* dst)
{
    rectf_t r_src = *src;
    rectf_t r_dst = *dst;
    ret_t ret = RET_OK;
    bool_t is_opaque = (img->flags & BITMAP_FLAG_OPAQUE || img->format == BITMAP_FMT_RGB565);

    if (img->format == fb->format && is_opaque &&    \
        src->w == dst->w && src->h == dst->h &&     \
        g_alpha >= ALPHA_OPACITY) 
    {
        xy_t dx = r_dst.x;
        xy_t dy = r_dst.y;
        rect_t r = rect_from_rectf(&r_src);
        ret = image_copy(fb, img, &r, dx, dy);
    } 
#if 0
    else
    {
   
        if (sys_info()->flags & SYSTEM_INFO_FLAG_FAST_LCD_PORTRAIT) 
        {
            if (o == LCD_ORIENTATION_0 || is_img_orientation) 
            {
                ret = image_blend(&fb, img, &r_dst, &r_src, lcd->global_alpha);
            } 
            else 
            {
                ret = image_rotate_blend(&fb, img, &r_dst, &r_src, lcd->global_alpha, o);
            }
        } 
        else
        {
          ret = image_blend(fb, img, &r_dst, &r_src, lcd->global_alpha);
        }
    }
#else       
    else 
    {
        ret = image_blend(fb, img, &r_dst, &r_src, g_alpha);
    }
#endif
    return ret;
}

ret_t mem_stroke_rect_with_line_w(bitmap_t* fb, rect_t* rect, int line_w, color_t color)
{
    int i;
    
    return_value_if_fail(fb != NULL && fb->buffer != NULL && rect->w > 0 && rect->h > 0 && line_w > 0, RET_BAD_PARAMS);

    for (i = 0; i < line_w; i++)
    {
        rect_t r_dst = {rect->x + i, rect->y + i, rect->w - (2 * i), rect->h - (2 * i)};
        mem_stroke_rect(fb, &r_dst, color);
    }
            
    return RET_OK;
}

ret_t mem_stroke_rect(bitmap_t* fb, rect_t* rect, color_t color)
{
    return_value_if_fail(fb != NULL && fb->buffer != NULL && rect->w > 0 && rect->h > 0, RET_BAD_PARAMS);

    mem_draw_hline(fb, rect->x, rect->y, rect->w, color);
    mem_draw_hline(fb, rect->x, rect->y + rect->h - 1, rect->w, color);
    mem_draw_vline(fb, rect->x, rect->y, rect->h, color);
    mem_draw_vline(fb, rect->x + rect->w - 1, rect->y, rect->h, color);
    
    return RET_OK;
}

ret_t mem_fill_rect(bitmap_t* fb, xy_t x, xy_t y, wh_t w, wh_t h, color_t color)
{
    rect_t rr;

    rr = rect_init(x, y, w, h);
    color.rgba.a = (color.rgba.a * g_alpha) / 0xff;
    
    return image_fill(fb, &rr, color);
}

ret_t mem_clear_rect(bitmap_t* fb, xy_t x, xy_t y, wh_t w, wh_t h, color_t color)
{
    rect_t rr;
    
    rr = rect_init(x, y, w, h);
    color.rgba.a = (color.rgba.a * g_alpha) / 0xff;
    
    return image_clear(fb, &rr, color);
}

ret_t mem_draw_hline(bitmap_t* fb, xy_t x, xy_t y, wh_t w, color_t color)
{
    return mem_fill_rect(fb, x, y, w, 1, color);
}

ret_t mem_draw_vline(bitmap_t* fb, xy_t x, xy_t y, wh_t h, color_t color)
{
    wh_t i = 0;
    pixel_t* p = NULL;
    int32_t offset = 0;
    uint8_t alpha = (color.rgba.a * g_alpha) / 0xff;
    uint32_t line_length = fb->line_length;
    uint8_t* fbuff = fb->buffer;

    offset = line_length;
    p = (pixel_t*)(fbuff + y * line_length) + x;

    if (alpha >= ALPHA_OPACITY) 
    {
        pixel_t pixel = color_to_pixel(color);
        for (i = 0; i < h; i++) 
        {
            *p = pixel;
            p = (pixel_t*)(((char*)p) + offset);
        }
    } 
    else if (alpha >= ALPHA_TRANSPARENT) 
    {
        color.rgba.a = alpha;
        for (i = 0; i < h; i++) 
        {
            *p = blend_pixel(*p, color);
            p = (pixel_t*)(((char*)p) + offset);
        }
    }

    return RET_OK;
}

