
#ifndef _PLOT_MEM_H_
#define _PLOT_MEM_H_

#include "rect.h"

/* alpha 大于 ALPHA_OPACITY 的颜色认为是不透明颜色，不进行alpha混合。*/
#define ALPHA_OPACITY 0xfa

/* alpha 小于 ALPHA_TRANSPARENT 的颜色认为是透明颜色，不进行alpha混合，直接丢弃。*/
#define ALPHA_TRANSPARENT 0x02


#ifdef __cplusplus
extern "C" {
#endif


ret_t set_global_alpha(uint8_t alpha);
//ret_t mem_draw_glyph8(bitmap_t* fb, PT_Glyph glyph, const rect_t *src, xy_t x, xy_t y, color_t color);
ret_t mem_draw_points(bitmap_t* fb, point_t* points, uint32_t nr, color_t color);
ret_t mem_draw_image(bitmap_t* fb, bitmap_t* img, const rectf_t* src, const rectf_t* dst);
ret_t mem_stroke_rect_with_line_w(bitmap_t* fb, rect_t* rect, int line_w, color_t color);
ret_t mem_stroke_rect(bitmap_t* fb, rect_t* rect, color_t color);
ret_t mem_fill_rect(bitmap_t* fb, xy_t x, xy_t y, wh_t w, wh_t h, color_t color);
ret_t mem_clear_rect(bitmap_t* fb, xy_t x, xy_t y, wh_t w, wh_t h, color_t color);
ret_t mem_draw_hline(bitmap_t* fb, xy_t x, xy_t y, wh_t w, color_t color);
ret_t mem_draw_vline(bitmap_t* fb, xy_t x, xy_t y, wh_t h, color_t color);


#ifdef __cplusplus
}
#endif

#endif /* _PLOT_MEM_H_ */

