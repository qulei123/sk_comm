#ifndef _SAVE_FILE_H_
#define _SAVE_FILE_H_

#include "bitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

void save_rgb_file(const unsigned char *src, int width, int height, int stride);
void save_rgb(char *name, const unsigned char *src, int height, int stride);
void save_ppm_file(const unsigned char *src, int width, int height, int stride);
void save_ppm(char *name, const unsigned char *src, int width, int height, int stride);
void save_jpg_file(const unsigned char *src, unsigned int src_size);
void save_yuv_file(const unsigned char *src, unsigned int src_size);

/* 将jpeg数据保存为jpg文件 */
int save_jpg_pic(bitmap_t *bitmap, char *name);
void save_jpg(const char *info, bitmap_t *bitmap);
int bitmap_to_mat(bitmap_t *bitmap, void *mat_bgr);

#ifdef __cplusplus
}
#endif

#ifdef _OPENCV_
#include <opencv2/opencv.hpp>

int bitmap_to_mat(bitmap_t* bitmap, cv::Mat& m_bgr);

#endif


#endif /*  _SAVE_FILE_H_ */

