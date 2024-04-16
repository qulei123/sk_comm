/**
 *  参考：v4l-utils-1.22.1\lib\libv4lconvert\rgbyuv.c
 */

#include <string.h>

#include "tools.h"
#include "cv_color.h"

/**图像格式说明： 
 * 1. RGBA32 存储使用32位RGB格式的图像, 内存存储顺序为RGBA (内存地址递增)  --> 0xaabbggrr (小端)
 * 2. RGB32/ARGB32/XRGB32 存储使用32位RGB格式的图像, 内存存储顺序为 ARGB (内存地址递增)  --> 0xbbggrraa
 * 3. RGB24 存储使用24位RGB格式的图像, 内存存储顺序为  RGB (内存地址递增)
 * 4. RGB16 存储使用16位RGB格式的图像，内存存储顺序为 g2g1g0b4b3b2b1b0 r4r3r2r1r0g5g4g3  --> ((uint16_t)rrrrrggg gggbbbbb)
 */

#define CLIP(color)     (unsigned char)(((color) > 0xFF) ? 0xff : (((color) < 0) ? 0 : (color)))
#define FLAG_MIRROR     1


void cv_rgb24_crop(unsigned char *src, const rect_t* s_rect, unsigned char *dst, const rect_t* d_rect, int flag)
{
    int i;
    int crop_x = d_rect->x;
    int crop_y = d_rect->y;
    int crop_w = min(d_rect->w, (s_rect->w - crop_x));
    int crop_h = min(d_rect->h, (s_rect->h - crop_y));
    unsigned char *tmp_src;
    unsigned char *tmp_dst;
    unsigned int stride_src = s_rect->w * 3;
    unsigned int stride_dst = crop_w * 3;

    for (i = 0; i < crop_h; i++)
    {
        tmp_src = src + (crop_y + i) * stride_src + crop_x * 3;
        tmp_dst = dst + i * stride_dst;

        if (CV_FLAG_MIRROR == flag)
        {
            for(int j = 0; j < crop_w; j++)
            {
                unsigned char *tmp = tmp_dst + (crop_w - j - 1) * 3;
                *tmp++ = *tmp_src++;        /* r */
                *tmp++ = *tmp_src++;        /* g */
                *tmp++ = *tmp_src++;        /* b */
            }
        }
        else
        {
            memcpy(tmp_dst, tmp_src, stride_dst);
        }
    }
}

/* YUYV(YUV 4:2:2) --> RGB24(RGB-8-8-8) */
void cv_yuyv_to_crop_rgb24(unsigned char *src, const rect_t* s_rect, int stride, unsigned char *dst, const rect_t* d_rect)
{
    int i, j;
    int width  = s_rect->w;
    int height = s_rect->h;
    int crop_x = d_rect->x;
    int crop_y = d_rect->y;
    unsigned char *tmp_src;
#if FLAG_MIRROR
    unsigned char *tmp_dst;
#endif

    if (width >= crop_x + d_rect->w)
    {
        width = crop_x + d_rect->w;
    }
    
    if (height >= (crop_y + d_rect->h))
    {
        height = crop_y + d_rect->h;
    }
#if FLAG_MIRROR
    int k = 0;
#endif
    for (i = crop_y; i < height; i++)
    {
        tmp_src = src + i * stride + crop_x * 2;
#if FLAG_MIRROR
        tmp_dst = dst + (k + 1) * (d_rect->w * 3) - 1;
#endif

        for (j = crop_x; j + 1 < width; j += 2) 
        {
            int u = tmp_src[1];
            int v = tmp_src[3];
            int u1 = (((u - 128) << 7) +  (u - 128)) >> 6;
            int rg = (((u - 128) << 1) +  (u - 128) + ((v - 128) << 2) + ((v - 128) << 1)) >> 3;
            int v1 = (((v - 128) << 1) +  (v - 128)) >> 1;

#if FLAG_MIRROR
            /* 解决显示mirror问题 */
            *tmp_dst-- = CLIP(tmp_src[0] + u1);
            *tmp_dst-- = CLIP(tmp_src[0] - rg);
            *tmp_dst-- = CLIP(tmp_src[0] + v1);
            
            *tmp_dst-- = CLIP(tmp_src[2] + u1);
            *tmp_dst-- = CLIP(tmp_src[2] - rg);
            *tmp_dst-- = CLIP(tmp_src[2] + v1);
#else
            *dst++ = CLIP(tmp_src[0] + v1);    /* r */
            *dst++ = CLIP(tmp_src[0] - rg);    /* g */
            *dst++ = CLIP(tmp_src[0] + u1);    /* b */

            *dst++ = CLIP(tmp_src[2] + v1);    /* r */
            *dst++ = CLIP(tmp_src[2] - rg);    /* g */
            *dst++ = CLIP(tmp_src[2] + u1);    /* b */
#endif
            tmp_src += 4;
        }
#if FLAG_MIRROR
        k++;
#endif
    }
}

/* YUYV(YUV 4:2:2) --> RGB24(RGB-8-8-8) */
void cv_yuyv_to_rgb24(const unsigned char *src, unsigned char *dst, int width, int height, int stride)
{
    int j;
    int pad = stride - (width * 2);

    while (--height >= 0) 
    {
        for (j = 0; j + 1 < width; j += 2) 
        {
            int u = src[1];
            int v = src[3];
            int u1 = (((u - 128) << 7) +  (u - 128)) >> 6;
            int rg = (((u - 128) << 1) +  (u - 128) + ((v - 128) << 2) + ((v - 128) << 1)) >> 3;
            int v1 = (((v - 128) << 1) +  (v - 128)) >> 1;

            *dst++ = CLIP(src[0] + v1);    /* r */
            *dst++ = CLIP(src[0] - rg);    /* g */
            *dst++ = CLIP(src[0] + u1);    /* b */

            *dst++ = CLIP(src[2] + v1);    /* r */
            *dst++ = CLIP(src[2] - rg);    /* g */
            *dst++ = CLIP(src[2] + u1);    /* b */
            src += 4;
        }
        src += pad;                         /* 跳过每行无用的字节 */
    }
}


void cv_yvyu_to_rgb24(const unsigned char *src, unsigned char *dst, int width, int height, int stride)   /* yvyu的stride */
{
    int j;
    int pad = stride - (width * 2);
    
    while (--height >= 0)
    {
        for (j = 0; j + 1 < width; j += 2)
        {
            int u = src[3];
            int v = src[1];
            int u1 = (((u - 128) << 7) +  (u - 128)) >> 6;
            int rg = (((u - 128) << 1) +  (u - 128) + ((v - 128) << 2) + ((v - 128) << 1)) >> 3;
            int v1 = (((v - 128) << 1) +  (v - 128)) >> 1;

            *dst++ = CLIP(src[0] + v1);    /* r */
            *dst++ = CLIP(src[0] - rg);    /* g */
            *dst++ = CLIP(src[0] + u1);    /* b */

            *dst++ = CLIP(src[2] + v1);    /* r */
            *dst++ = CLIP(src[2] - rg);    /* g */
            *dst++ = CLIP(src[2] + u1);    /* b */
            src += 4;
        }
        src += pad;                         /* 跳过每行无用的字节 */
    }
}
 
/* RGBA32(RGBA-8-8-8-8) --> RGB24(RGB-8-8-8) */
void cv_rgba32_to_rgb24(const unsigned char *src, unsigned char *dst, int width, int height)
{
    int j;
    
    while (--height >= 0) 
    {
        for (j = 0; j < width; j++) 
        {
            *dst++ = *src++;       /* r */
            *dst++ = *src++;       /* g */
            *dst++ = *src++;       /* b */
            src += 1;              /* a */
        }
    }
}

/* RGB24(RGB-8-8-8) --> RGBA32(RGBA-8-8-8-8) */
void cv_rgb24_to_rgba32(const unsigned char *src, unsigned char *dst, int width, int height)
{
    int j;
    
    while (--height >= 0) 
    {
        for (j = 0; j < width; j++) 
        {
            *dst++ = *src++;       /* r */
            *dst++ = *src++;       /* g */
            *dst++ = *src++;       /* b */
            *dst++ = 0xff;         /* a */
        }
    }
}

/* RGB24(RGB-8-8-8) --> BGRA32(BGRA-8-8-8-8) */
void cv_rgb24_to_bgra32(const unsigned char *src, unsigned char *dst, int width, int height)
{
    int j;
    
    while (--height >= 0) 
    {
        for (j = 0; j < width; j++) 
        {
            *dst++ = src[2];       /* b */
            *dst++ = src[1];       /* g */
            *dst++ = src[0];       /* r */
            *dst++ = 0xff;         /* a */
            src += 3;
        }
    }
}

