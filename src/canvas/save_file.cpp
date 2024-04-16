/**
 *  通用写文件操作
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "save_file.h"


#define NOT_IMPL    "Do not support the opencv, this function is not implemented\n"


/* 将rgb数据保存为rgb文件 */
void save_rgb_file(const unsigned char *src, int width, int height, int stride)
{
    static int i = 0;
    char out_name[32];
    
    sprintf(out_name, "/opt/out/%04d_%d_%d.rgb", i++, width, height);
    save_rgb(out_name, src, height, stride);
}

void save_rgb(char *name, const unsigned char *src, int height, int stride)
{
    FILE *fout;
    
    fout = fopen(name, "w");
    if (!fout)
    {
        perror("fopen rgb fail");
        return;      
    }
    
    fwrite(src, height * stride, 1, fout);
    fclose(fout);
}

/* 将rgb数据保存为ppm文件 */
void save_ppm_file(const unsigned char *src, int width, int height, int stride)
{
    static int i = 0;
    char out_name[32];
    
    sprintf(out_name, "/opt/out/%04d.ppm", i++);
    save_ppm(out_name, src, width, height, stride);
}

void save_ppm(char *name, const unsigned char *src, int width, int height, int stride)
{
    FILE *fout;
    
    fout = fopen(name, "w");
    if (!fout)
    {
        perror("fopen ppm fail");
        return;      
    }
    
    fprintf(fout, "P6\n%d %d 255\n", width, height);
    fwrite(src, height * stride, 1, fout);
    fclose(fout);
}

static void save_file(const unsigned char *src, int src_size, const char *fmt)
{
    static int i = 0;
    char out_name[32];
    int fd, writesize;
    
    sprintf(out_name, "/opt/out/%04d.%s", i++, fmt);
    fd = open(out_name, O_RDWR | O_CREAT, 00700);
    if(fd == -1)
    {
        perror("fopen jpg fail");
        return;      
    }
    
    writesize = write(fd, src, src_size);
    if (writesize < src_size)
    {
        printf("write %s file err [%d < %d]\n", fmt, writesize, src_size);
    }
    close(fd);
}

/* 将jpeg数据保存为jpg文件 */
void save_jpg_file(const unsigned char *src, unsigned int src_size)
{
    save_file(src, src_size, "jpg");
}

/* 将yuv数据保存为yuv文件 */
void save_yuv_file(const unsigned char *src, unsigned int src_size)
{
    save_file(src, src_size, "yuv");
}


#ifdef _OPENCV_

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;


int bitmap_to_mat(bitmap_t *bitmap, Mat &m_bgr)
{
    ENSURE(NULL != bitmap);
    uint16_t format = bitmap->format;

    if (BITMAP_FMT_RGB888 == format)
    {
        Mat m_rgb(bitmap->h, bitmap->w, CV_8UC3, bitmap->buffer);
        cvtColor(m_rgb, m_bgr, COLOR_RGB2BGR);
    } 
    else if (BITMAP_FMT_BGRA8888 == format)
    {
        Mat m_bgra(bitmap->h, bitmap->w, CV_8UC4, bitmap->buffer);
        cvtColor(m_bgra, m_bgr, COLOR_BGRA2BGR);
    }
    else
    {
        printf("not support pic fmt %d.\n", format);
        return 0;
    }

    return 1;
}

/* 兼容c接口调用，使用指针传递Mat */
int bitmap_to_mat(bitmap_t *bitmap, void *mat_bgr)
{
    ENSURE(NULL != bitmap);
    Mat *m_bgr = static_cast<Mat *>(mat_bgr);
    uint16_t format = bitmap->format;

    if (BITMAP_FMT_RGB888 == format)
    {
        Mat m_rgb(bitmap->h, bitmap->w, CV_8UC3, bitmap->buffer);
        cvtColor(m_rgb, *m_bgr, COLOR_RGB2BGR);
    } 
    else if (BITMAP_FMT_BGRA8888 == format)
    {
        Mat m_bgra(bitmap->h, bitmap->w, CV_8UC4, bitmap->buffer);
        cvtColor(m_bgra, *m_bgr, COLOR_BGRA2BGR);
    }
    else
    {
        printf("not support pic fmt %d.\n", format);
        return 0;
    }

    return 1;
}

/* 将jpeg数据保存为jpg文件 */
int save_jpg_pic(bitmap_t *bitmap, char *name)
{
    ENSURE(NULL != bitmap);
    Mat m_bgr(bitmap->h, bitmap->w, CV_8UC3);

#if 0
    uint16_t format = bitmap->format;
    if (BITMAP_FMT_RGB888 == format)
    {
        Mat m_rgb(bitmap->h, bitmap->w, CV_8UC3);
        memcpy(m_rgb.data, bitmap->buffer, bitmap->w * bitmap->h * 3);
        cvtColor(m_rgb, m_bgr, COLOR_RGB2BGR);
    } 
    else if (BITMAP_FMT_BGRA8888 == format)
    {
        Mat m_bgra(bitmap->h, bitmap->w, CV_8UC4);
        memcpy(m_bgra.data, bitmap->buffer, bitmap->w * bitmap->h * 4);
        cvtColor(m_bgra, m_bgr, COLOR_BGRA2BGR);
    }
    else
    {
        printf("not support pic fmt %d.\n", format);
        return 0;
    }
#else
    bitmap_to_mat(bitmap, &m_bgr);
#endif

    vector<int> params;
    params.push_back(IMWRITE_JPEG_QUALITY);
    params.push_back(75);

    imwrite(name, m_bgr, params);

    return 1;
}

void save_jpg(const char *info, bitmap_t *bitmap)
{
    static int i = 0;
    char out_name[32];
    
    sprintf(out_name, "/opt/out/%s_%04d.jpg", info, i++);
    printf("%s\n", out_name);
    save_jpg_pic(bitmap, out_name);
}

#else

int save_jpg_pic(bitmap_t *bitmap, char *name)
{
    log_war(NOT_IMPL);
    return 1;
}

void save_jpg(const char *info, bitmap_t *bitmap)
{
    log_war(NOT_IMPL);
}

int bitmap_to_mat(bitmap_t *bitmap, void *mat_bgr)
{
    log_war(NOT_IMPL);
    return 1;
}

#endif

