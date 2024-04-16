#ifndef _CV_COLOR_H_
#define _CV_COLOR_H_

#include "rect.h"

#ifdef __cplusplus
extern "C" {
#endif


#define CV_FLAG_NONE        0
#define CV_FLAG_MIRROR      1
#define CV_FLAG_FLIP        2

void cv_yuyv_to_rgb24(const unsigned char *src, unsigned char *dst, int width, int height, int stride);
void cv_yvyu_to_rgb24(const unsigned char *src, unsigned char *dst, int width, int height, int stride);
void cv_rgba32_to_rgb24(const unsigned char *src, unsigned char *dst, int width, int height);
void cv_rgb24_to_rgba32(const unsigned char *src, unsigned char *dst, int width, int height);   
void cv_rgb24_to_bgra32(const unsigned char *src, unsigned char *dst, int width, int height);
void cv_yuyv_to_crop_rgb24(unsigned char *src, const rect_t* s_rect, int stride, unsigned char *dst, const rect_t* d_rect);
void cv_rgb24_crop(unsigned char *src, const rect_t* s_rect, unsigned char *dst, const rect_t* d_rect, int flag);


#ifdef __cplusplus
}
#endif


#endif /* _CV_COLOR_H_ */

