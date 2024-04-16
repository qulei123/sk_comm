
#include <string.h>
#include "tools.h"

#define pixel_t pixel_dst_t
#define pixel_from_rgb pixel_dst_from_rgb


#ifndef blend_a
static inline void blend_a(uint8_t* dst, uint8_t* src, uint8_t alpha, bool_t premulti_alpha) 
{
    pixel_src_t s = *(pixel_src_t*)src;
    rgba_t srgba = pixel_src_to_rgba(s);
    uint8_t a = alpha == 0xff ? srgba.a : ((srgba.a * alpha) >> 8);

    /* 前景色的透明度大于 0xf8，则可以直接使用前景色 */
    if (a > 0xf8) 
    {
        pixel_dst_t p = pixel_dst_from_rgba(srgba.r, srgba.g, srgba.b, srgba.a);
        *(pixel_dst_t*)dst = p;
    }         
#if pixel_dst_bpp == 2
    else if (a > 0x08) 
#else
    else if (a > 0x0) 
#endif
    {
        pixel_dst_t d = *(pixel_dst_t*)dst;
        rgba_t drgba = pixel_dst_to_rgba(d);
#if pixel_dst_bpp == 2
        /* 背景色的透明度小于 0x08，则可以直接使用前景色 */
        if (drgba.a < 0x08) 
#else
        if (drgba.a == 0x0) 
#endif
        {
            pixel_dst_t p = pixel_dst_from_rgba(srgba.r, srgba.g, srgba.b, a);
            *(pixel_dst_t*)dst = p;
        } 
        else 
        {
            if (premulti_alpha) 
            {
                if(alpha <= 0xf8) 
                {
                    srgba.r = (srgba.r * alpha) >> 8;
                    srgba.g = (srgba.g * alpha) >> 8;
                    srgba.b = (srgba.b * alpha) >> 8;
                }
                *(pixel_dst_t*)dst = blend_rgba_premulti(drgba, srgba, 0xff - a);
            } 
            else  
            {
                *(pixel_dst_t*)dst = blend_rgba(drgba, srgba, a);
            }
        }
    }
}
#endif /*blend_a*/

#define BLEND_SCALE_IS_SAME_OFFSET(t) (((t) & 0xff) >= (1<<8) + 2 || ((t) & 0xff) <= 0x2)

static void get_circle_points(xy_t xc, xy_t yc, int x, int y, point_t *ptPoint)
{
    /* 8个点的，绘制4条水平线 */
    point_t atPoint[8] = {{-x + xc, y + yc}, {x + xc, y + yc},          /* (-x, y)  -> (x, y) */
                          {-y + xc, x + yc}, {y + xc, x + yc},          /* (-y, x)  -> (y, x) */
                          {-y + xc, -x + yc},{y + xc, -x + yc},         /* (-y, -x) -> (y, -x) */
                          {-x + xc, -y + yc},{x + xc, -y + yc}};        /* (-x, -y) -> (x, -y) */
    
    memcpy(ptPoint, atPoint, sizeof(atPoint));
}

static ret_t blend_image_with_alpha(bitmap_t* dst, bitmap_t* src, const rectf_t* dst_r, const rectf_t* src_r, uint8_t a) 
{
    wh_t i = 0;
    wh_t j = 0;
    xy_t sx = (xy_t)(src_r->x);
    xy_t sy = (xy_t)(src_r->y);
    wh_t sw = (wh_t)(src_r->w);
    wh_t sh = (wh_t)(src_r->h);
    xy_t dx = (xy_t)(dst_r->x);
    xy_t dy = (xy_t)(dst_r->y);
    wh_t dw = (wh_t)(dst_r->w);
    wh_t dh = (wh_t)(dst_r->h);
    wh_t src_iw = src->w;
    wh_t src_ih = src->h;
    wh_t dst_iw = dst->w;
    wh_t dst_ih = dst->h;
    uint8_t* srcp = NULL;
    uint8_t* dstp = NULL;
    uint8_t* src_data = NULL;
    uint8_t* dst_data = NULL;
    uint8_t src_bpp = bitmap_get_bpp(src);
    uint8_t dst_bpp = bitmap_get_bpp(dst);
    uint32_t src_line_length = src->line_length;
    uint32_t dst_line_length = dst->line_length;
    uint32_t src_line_offset = src_line_length - sw * src_bpp;
    uint32_t dst_line_offset = dst_line_length - dw * dst_bpp;
    bool_t premulti_alpha = src->flags & BITMAP_FLAG_PREMULTI_ALPHA;

    if (!(src_r->h > 0 && src_r->w > 0 && dst_r->h > 0 && dst_r->w > 0)) 
    {
        return RET_OK;
    }
    return_value_if_fail(sx >= 0 && sy >= 0 && (sx + sw) <= src_iw && (sy + sh) <= src_ih, RET_BAD_PARAMS);
    return_value_if_fail(dx >= 0 && dy >= 0 && (dx + dw) <= dst_iw && (dy + dh) <= dst_ih, RET_BAD_PARAMS);

    src_data = src->buffer;
    dst_data = dst->buffer;
    return_value_if_fail(src_data != NULL && dst_data != NULL, RET_BAD_PARAMS);

    srcp = src_data;
    dstp = dst_data;
    if (sw == dw && sh == dh) 
    {
        if (src->r == 0)
        {
            srcp += (sy * src_line_length + sx * src_bpp);
            dstp += (dy * dst_line_length + dx * dst_bpp);
            for (j = 0; j < dh; j++) 
            {
                for (i = 0; i < dw; i++) 
                {
                    blend_a(dstp, srcp, a, premulti_alpha);
                    dstp += dst_bpp;
                    srcp += src_bpp;
                }
                dstp += dst_line_offset;
                srcp += src_line_offset;
            }
        }
        else
        {
            /* src图片进行圆形裁剪处理 */
            int x = 0;
            int y = src->r;
            int d = 3 - 2 * src->r;
            point_t atPoint_s[8];
            point_t atPoint_d[8];
            xy_t start_x_s, start_x_d;
            xy_t start_y_s, start_y_d;
            xy_t end_x_s;
            //xy_t end_x_s, end_x_d;
            //xy_t end_y_s, end_y_d;
            int lw;
            
            while (x < y)
            {
                /* 目的矩形中心为圆心 */
                get_circle_points(sx + sw / 2, sy + sh / 2, x, y, atPoint_s);
                get_circle_points(dx + dw / 2, dy + dh / 2, x, y, atPoint_d);
                
                for (j = 0; j < 4; j++) 
                {
                    start_x_s = atPoint_s[j * 2].x;
                    start_y_s = atPoint_s[j * 2].y;
                    end_x_s = atPoint_s[j * 2 + 1].x;
                    //end_y_s = atPoint_s[j * 2 + 1].y;
                    
                    start_x_d = atPoint_d[j * 2].x;
                    start_y_d = atPoint_d[j * 2].y;
                    //end_x_d = atPoint_d[j * 2 + 1].x;
                    //end_y_d = atPoint_d[j * 2 + 1].y;

                    #if 0
                    srcp = src_data + (start_y_s * src_line_length + sx * src_bpp);
                    dstp = dst_data + (start_y_d * dst_line_length + dx * dst_bpp);
                    for (i = 0; i < dw; i++) 
                    {
                        if (i => (start_x_s - sx) && i <= (end_x_s - sx))
                        {
                            blend_a(dstp, srcp, a, premulti_alpha);
                        }
                        dstp += dst_bpp;
                        srcp += src_bpp;
                    }
                    #else
                    srcp = src_data + (start_y_s * src_line_length + start_x_s * src_bpp);
                    dstp = dst_data + (start_y_d * dst_line_length + start_x_d * dst_bpp);
                    lw = end_x_s - start_x_s;
                    for (i = 0; i <= lw; i++) 
                    {
                        blend_a(dstp, srcp, a, premulti_alpha);
                        dstp += dst_bpp;
                        srcp += src_bpp;
                    }
                    #endif
                }

                /*  */
                if(d < 0)
                {
                    d = d + 4 * x + 6;
                }
                else
                {
                    d = d + 4 * ( x - y ) + 10;
                    y--;
                }
                x++;
            }
        }
    } 
    else 
    {
        uint32_t right = max(dw, 1);
        uint32_t bottom = max(dh, 1);

        uint32_t scale_x = src_r->w * 256.0f / dst_r->w;
        uint32_t scale_y = src_r->h * 256.0f / dst_r->h;

        uint32_t p_x = 0;
        uint32_t p_y = 0;
        uint32_t start_y = 0;
        uint32_t start_x = 0;
        uint8_t* row_data = NULL;
        if (src_r->x != 0) 
        {
            start_x = (uint32_t)((src_r->x - sx) * 256.0f);
        }
        if (src_r->y != 0) 
        {
            start_y =  (uint32_t)((src_r->y - sy) * 256.0f);
        }

        dstp += (dy * dst_line_length + dx * dst_bpp);  /* dst的起始地址 */
        for (j = 0, p_y = start_y; j < bottom; j++, p_y += scale_y, dstp += dst_line_offset) 
        {
            uint32_t y = sy + (p_y >> 8);
            row_data = ((uint8_t*)(src_data)) + y * src_line_length + sx * src_bpp;

            for (i = 0, p_x = start_x; i < right; i++, p_x += scale_x, dstp += dst_bpp) 
            {
                uint32_t x = p_x >> 8;

                srcp = row_data + x * src_bpp;
                blend_a(dstp, srcp, a, premulti_alpha);
            }
        }
    }
    
    return RET_OK;
}

static ret_t blend_image_without_alpha(bitmap_t* dst, bitmap_t* src, const rectf_t* dst_r, const rectf_t* src_r) 
{
    return blend_image_with_alpha(dst, src, dst_r, src_r, 0xff);
}

#if 0
static ret_t blend_image_rotate_90_acw_with_alpha(bitmap_t* dst, bitmap_t* src, const rectf_t* dst_r, const rectf_t* src_r, uint8_t a) 
{
  wh_t i = 0;
  wh_t j = 0;
  wh_t sw = (wh_t)(src_r->w);
  wh_t sh = (wh_t)(src_r->h);
  xy_t sx = (xy_t)(src_r->x);
  xy_t sy = (xy_t)(src_r->y);
  wh_t src_iw = bitmap_get_physical_width(src);
  wh_t src_ih = bitmap_get_physical_height(src);
  wh_t dst_iw = bitmap_get_physical_width(dst);
  wh_t dst_ih = bitmap_get_physical_height(dst);
  uint8_t* srcp = NULL;
  uint8_t* dstp = NULL;
  uint8_t* src_data = NULL;
  uint8_t* dst_data = NULL;
  xy_t dx = 0, dy = 0, dw = 0, dh = 0;
  uint8_t src_bpp = bitmap_get_bpp(src);
  uint8_t dst_bpp = bitmap_get_bpp(dst);
  uint32_t src_line_length = bitmap_get_physical_line_length(src);
  uint32_t dst_line_length = bitmap_get_physical_line_length(dst);
  bool_t premulti_alpha = src->flags & BITMAP_FLAG_PREMULTI_ALPHA;
  rectf_t r_dst = lcd_orientation_rectf_rotate_by_anticlockwise(dst_r, LCD_ORIENTATION_90, dst_ih, dst_iw);
  
  dx = (xy_t)(r_dst.x);
  dy = (xy_t)(r_dst.y);
  dw = (wh_t)(r_dst.w);
  dh = (wh_t)(r_dst.h);
  if (!(src_r->h > 0 && src_r->w > 0 && dst_r->h > 0 && dst_r->w > 0)) {
    return RET_OK;
  }
  return_value_if_fail(sx >= 0 && sy >= 0 && (sx + sw) <= src_iw && (sy + sh) <= src_ih,
                       RET_BAD_PARAMS);
  return_value_if_fail(dx >= 0 && dy >= 0 && (dx + dw) <= dst_iw && (dy + dh) <= dst_ih,
                       RET_BAD_PARAMS);

  src_data = bitmap_lock_buffer_for_read(src);
  dst_data = bitmap_lock_buffer_for_write(dst);
  return_value_if_fail(src_data != NULL && dst_data != NULL, RET_BAD_PARAMS);

  srcp = src_data;
  dstp = dst_data;
  if (sw == (wh_t)dst_r->w && sh == (wh_t)dst_r->h) {
    pixel_src_t* src_p = (pixel_src_t*)(srcp + sy * src_line_length + sx * src_bpp);
    pixel_dst_t* dst_p = (pixel_dst_t*)(dstp + dy * dst_line_length + dx * dst_bpp);

    for (i = 0; i < sh && i < dw; i++) {
      pixel_src_t* s = src_p + sw - 1;
      pixel_dst_t* d = dst_p;

      for (j = 0; j < sw && j < dh; j++, s--) {
        blend_a((uint8_t*)d, (uint8_t*)s, a, premulti_alpha);
        d = (pixel_dst_t*)(((char*)d) + dst_line_length);
      }
      dst_p++;
      src_p = (pixel_src_t*)(((char*)src_p) + src_line_length);
    }
  } else {
    uint32_t right = tk_max(dh, 1);
    uint32_t bottom = tk_max(dw, 1);

    uint32_t scale_x = src_r->w * 256.0f / dst_r->w;
    uint32_t scale_y = src_r->h * 256.0f / dst_r->h;

    uint32_t p_x = 0;
    uint32_t p_y = 0;
    uint32_t start_y = 0;
    uint32_t start_x = 0;
    uint8_t* row_data = NULL;
    if (src_r->x != 0) {
      start_x = (uint32_t)((src_r->x - sx) * 256.0f);
    }
    if (src_r->y != 0) {
      start_y =  (uint32_t)((src_r->y - sy) * 256.0f);
    }

    dstp += ((dy + dh - 1) * dst_line_length + dx * dst_bpp);
    for (j = 0, p_y = start_y; j < bottom; j++, p_y += scale_y) {
      uint8_t* d = dstp;
      uint32_t y = sy + (p_y >> 8);
      row_data = ((uint8_t*)(src_data)) + y * src_line_length + sx * src_bpp;

      for (i = 0, p_x = start_x; i < right; i++, p_x += scale_x) {
        uint32_t x = p_x >> 8;

        srcp = row_data + x * src_bpp;
        blend_a(d, srcp, a, premulti_alpha);
        d -= dst_line_length;
      }
      dstp += dst_bpp;
    }
  }
  bitmap_unlock_buffer(src);
  bitmap_unlock_buffer(dst);

  return RET_OK;
}

static ret_t blend_image_rotate_180_acw_with_alpha(bitmap_t* dst, bitmap_t* src, const rectf_t* dst_r, const rectf_t* src_r, uint8_t a) 
{
  wh_t i = 0;
  wh_t j = 0;
  wh_t sw = (wh_t)(src_r->w);
  wh_t sh = (wh_t)(src_r->h);
  xy_t sx = (xy_t)(src_r->x);
  xy_t sy = (xy_t)(src_r->y);
  wh_t src_iw = bitmap_get_physical_width(src);
  wh_t src_ih = bitmap_get_physical_height(src);
  wh_t dst_iw = bitmap_get_physical_width(dst);
  wh_t dst_ih = bitmap_get_physical_height(dst);
  uint8_t* srcp = NULL;
  uint8_t* dstp = NULL;
  uint8_t* src_data = NULL;
  uint8_t* dst_data = NULL;
  xy_t dx = 0, dy = 0, dw = 0, dh = 0;
  uint8_t src_bpp = bitmap_get_bpp(src);
  uint8_t dst_bpp = bitmap_get_bpp(dst);
  uint32_t src_line_length = bitmap_get_physical_line_length(src);
  uint32_t dst_line_length = bitmap_get_physical_line_length(dst);
  bool_t premulti_alpha = src->flags & BITMAP_FLAG_PREMULTI_ALPHA;
  rectf_t r_dst = lcd_orientation_rectf_rotate_by_anticlockwise(dst_r, LCD_ORIENTATION_180, dst_iw, dst_ih);
  
  dx = (xy_t)(r_dst.x);
  dy = (xy_t)(r_dst.y);
  dw = (wh_t)(r_dst.w);
  dh = (wh_t)(r_dst.h);
  if (!(src_r->h > 0 && src_r->w > 0 && dst_r->h > 0 && dst_r->w > 0)) {
    return RET_OK;
  }
  return_value_if_fail(sx >= 0 && sy >= 0 && (sx + sw) <= src_iw && (sy + sh) <= src_ih,
                       RET_BAD_PARAMS);
  return_value_if_fail(dx >= 0 && dy >= 0 && (dx + dw) <= dst_iw && (dy + dh) <= dst_ih,
                       RET_BAD_PARAMS);

  src_data = bitmap_lock_buffer_for_read(src);
  dst_data = bitmap_lock_buffer_for_write(dst);
  return_value_if_fail(src_data != NULL && dst_data != NULL, RET_BAD_PARAMS);

  srcp = src_data;
  dstp = dst_data;
  if (sw == (wh_t)dst_r->w && sh == (wh_t)dst_r->h) {
    pixel_src_t* src_p = (pixel_src_t*)(srcp + sy * src_line_length + sx * src_bpp);
    pixel_dst_t* dst_p = (pixel_dst_t*)(dstp + (dy + dh - 1) * dst_line_length + (dx + dw - 1) * dst_bpp);

    for (i = 0; i < sh && i < dh; i++) {
      pixel_src_t* s = src_p;
      pixel_dst_t* d = dst_p;
      for (j = 0; j < sw && j < dw; j++) {
        blend_a((uint8_t*)d, (uint8_t*)s, a, premulti_alpha);
        d--;
        s++;
      }
      dst_p = (pixel_dst_t*)(((char*)dst_p) - dst_line_length);
      src_p = (pixel_src_t*)(((char*)src_p) + src_line_length);
    }
  } else {
    uint32_t right = tk_max(dw, 1);
    uint32_t bottom = tk_max(dh, 1);

    uint32_t scale_x = src_r->w * 256.0f / dst_r->w;
    uint32_t scale_y = src_r->h * 256.0f / dst_r->h;

    uint32_t p_x = 0;
    uint32_t p_y = 0;
    uint32_t start_y = 0;
    uint32_t start_x = 0;
    uint8_t* row_data = NULL;
    if (src_r->x != 0) {
      start_x = (uint32_t)((src_r->x - sx) * 256.0f);
    }
    if (src_r->y != 0) {
      start_y =  (uint32_t)((src_r->y - sy) * 256.0f);
    }

    dstp += ((dy + dh - 1) * dst_line_length + (dx + dw - 1) * dst_bpp);
    for (j = 0, p_y = start_y; j < bottom; j++, p_y += scale_y) {
      uint8_t* d = dstp;
      uint32_t y = sy + (p_y >> 8);
      row_data = ((uint8_t*)(src_data)) + y * src_line_length + sx * src_bpp;

      for (i = 0, p_x = start_x; i < right; i++, p_x += scale_x) {
        uint32_t x = p_x >> 8;

        srcp = row_data + x * src_bpp;
        blend_a(d, srcp, a, premulti_alpha);
        d -= dst_bpp;
      }
      dstp -= dst_line_length;
    }
  }
  bitmap_unlock_buffer(src);
  bitmap_unlock_buffer(dst);
  return RET_OK;
}

static ret_t blend_image_rotate_270_acw_with_alpha(bitmap_t* dst, bitmap_t* src, const rectf_t* dst_r, const rectf_t* src_r, uint8_t a) 
{
  wh_t i = 0;
  wh_t j = 0;
  wh_t sw = (wh_t)(src_r->w);
  wh_t sh = (wh_t)(src_r->h);
  xy_t sx = (xy_t)(src_r->x);
  xy_t sy = (xy_t)(src_r->y);
  wh_t src_iw = bitmap_get_physical_width(src);
  wh_t src_ih = bitmap_get_physical_height(src);
  wh_t dst_iw = bitmap_get_physical_width(dst);
  wh_t dst_ih = bitmap_get_physical_height(dst);
  uint8_t* srcp = NULL;
  uint8_t* dstp = NULL;
  uint8_t* src_data = NULL;
  uint8_t* dst_data = NULL;
  xy_t dx = 0, dy = 0, dw = 0, dh = 0;
  uint8_t src_bpp = bitmap_get_bpp(src);
  uint8_t dst_bpp = bitmap_get_bpp(dst);
  uint32_t src_line_length = bitmap_get_physical_line_length(src);
  uint32_t dst_line_length = bitmap_get_physical_line_length(dst);
  bool_t premulti_alpha = src->flags & BITMAP_FLAG_PREMULTI_ALPHA;
  rectf_t r_dst = lcd_orientation_rectf_rotate_by_anticlockwise(dst_r, LCD_ORIENTATION_270, dst_ih, dst_iw);
  
  dx = (xy_t)(r_dst.x);
  dy = (xy_t)(r_dst.y);
  dw = (wh_t)(r_dst.w);
  dh = (wh_t)(r_dst.h);
  if (!(src_r->h > 0 && src_r->w > 0 && dst_r->h > 0 && dst_r->w > 0)) {
    return RET_OK;
  }
  return_value_if_fail(sx >= 0 && sy >= 0 && (sx + sw) <= src_iw && (sy + sh) <= src_ih,
                       RET_BAD_PARAMS);
  return_value_if_fail(dx >= 0 && dy >= 0 && (dx + dw) <= dst_iw && (dy + dh) <= dst_ih,
                       RET_BAD_PARAMS);

  src_data = bitmap_lock_buffer_for_read(src);
  dst_data = bitmap_lock_buffer_for_write(dst);
  return_value_if_fail(src_data != NULL && dst_data != NULL, RET_BAD_PARAMS);

  srcp = src_data;
  dstp = dst_data;
  if (sw == (wh_t)dst_r->w && sh == (wh_t)dst_r->h) {
    pixel_src_t* src_p = (pixel_src_t*)(srcp + sy * src_line_length + sx * src_bpp);
    pixel_dst_t* dst_p = (pixel_dst_t*)(dstp + dy * dst_line_length + (dx + dw - 1) * dst_bpp);

    for (i = 0; i < sh && i < dw; i++) {
      pixel_src_t* s = src_p;
      pixel_dst_t* d = dst_p;

      for (j = 0; j < sw && j < dh; j++, s++) {
        blend_a((uint8_t*)d, (uint8_t*)s, a, premulti_alpha);
        d = (pixel_dst_t*)(((char*)d) + dst_line_length);
      }
      dst_p--;
      src_p = (pixel_src_t*)(((char*)src_p) + src_line_length);
    }
  } else {
    uint32_t right = tk_max(dh, 1);
    uint32_t bottom = tk_max(dw, 1);

    uint32_t scale_x = src_r->w * 256.0f / dst_r->w;
    uint32_t scale_y = src_r->h * 256.0f / dst_r->h;

    uint32_t p_x = 0;
    uint32_t p_y = 0;
    uint32_t start_y = 0;
    uint32_t start_x = 0;
    uint8_t* row_data = NULL;
    if (src_r->x != 0) {
      start_x = (uint32_t)((src_r->x - sx) * 256.0f);
    }
    if (src_r->y != 0) {
      start_y =  (uint32_t)((src_r->y - sy) * 256.0f);
    }

    dstp += (dy * dst_line_length + (dx + dw - 1) * dst_bpp);
    for (j = 0, p_y = start_y; j < bottom; j++, p_y += scale_y) {
      uint8_t* d = dstp;
      uint32_t y = sy + (p_y >> 8);
      row_data = ((uint8_t*)(src_data)) + y * src_line_length + sx * src_bpp;

      for (i = 0, p_x = start_x; i < right; i++, p_x += scale_x) {
        uint32_t x = p_x >> 8;

        srcp = row_data + x * src_bpp;
        blend_a(d, srcp, a, premulti_alpha);
        d += dst_line_length;
      }
      dstp -= dst_bpp;
    }
  }
  bitmap_unlock_buffer(src);
  bitmap_unlock_buffer(dst);

  return RET_OK;
}


static ret_t blend_image_with_alpha_by_rotate(bitmap_t* dst, bitmap_t* src, const rectf_t* dst_r, const rectf_t* src_r,
                                    uint8_t a, lcd_orientation_t o) 
{
  switch (o)
  {
  case LCD_ORIENTATION_0:
    return blend_image_with_alpha(dst, src, dst_r, src_r, a);
  case LCD_ORIENTATION_90 :
    return blend_image_rotate_90_acw_with_alpha(dst, src, dst_r, src_r, a);
  case LCD_ORIENTATION_180:
    return blend_image_rotate_180_acw_with_alpha(dst, src, dst_r, src_r, a);
  case LCD_ORIENTATION_270:
    return blend_image_rotate_270_acw_with_alpha(dst, src, dst_r, src_r, a);
  default:
    break;
  }
  return RET_NOT_IMPL;
}
#endif

