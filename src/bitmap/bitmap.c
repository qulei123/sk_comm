
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "bitmap.h"
#include "tools.h"

#define BITMAP_MONO_LINE_LENGTH(w) (((w + 15) >> 4) << 1)


bitmap_t* bitmap_create(void) 
{
    bitmap_t* bitmap = calloc(1, sizeof(bitmap_t));
    return_value_if_fail(bitmap != NULL, NULL);

    bitmap->should_free_handle = TRUE;

    return bitmap;
}

ret_t bitmap_destroy_with_self(bitmap_t* bitmap) 
{
    return_value_if_fail(bitmap != NULL, RET_BAD_PARAMS);
    bitmap->should_free_handle = TRUE;

    return bitmap_destroy(bitmap);
}

ret_t bitmap_destroy(bitmap_t* bitmap) 
{
    return_value_if_fail(bitmap != NULL, RET_BAD_PARAMS);

    if (bitmap->should_free_data) 
    {
        if (bitmap->buffer != NULL) 
        {
            free(bitmap->buffer);
            bitmap->buffer = NULL;
        }
    }
    
    if (bitmap->should_free_handle) 
    {
        memset(bitmap, 0x00, sizeof(bitmap_t));
        free(bitmap);
        bitmap = NULL;
    } 
    else 
    {
        memset(bitmap, 0x00, sizeof(bitmap_t));
    }

    return RET_OK;
}

uint32_t bitmap_get_size(uint32_t w, uint32_t h, bitmap_format_t format)
{
    uint32_t line_length = 0;
    uint32_t bpp, size;

    if (format == BITMAP_FMT_MONO)
    {
        line_length = BITMAP_MONO_LINE_LENGTH(w);
    }
    else
    {
        bpp = bitmap_get_bpp_of_format(format);
        line_length = max(w * bpp, line_length);
    }

    size = line_length * h;
    size = ROUND_TO(size, BITMAP_ALIGN_SIZE) + BITMAP_ALIGN_SIZE;

    return size;
}

uint32_t bitmap_get_bpp_of_format(bitmap_format_t format)
{
    switch (format)
    {
        case BITMAP_FMT_RGBA8888:
        case BITMAP_FMT_ABGR8888:
        case BITMAP_FMT_BGRA8888:
        case BITMAP_FMT_ARGB8888:
            return 4;
        case BITMAP_FMT_RGB565:
        case BITMAP_FMT_BGR565:
            return 2;
        case BITMAP_FMT_RGB888:
        case BITMAP_FMT_BGR888:
            return 3;
        case BITMAP_FMT_GRAY:
            return 1;
        default:
            break;
    }

    return 0;
}

uint32_t bitmap_get_bpp(bitmap_t* bitmap) 
{
    return_value_if_fail(bitmap != NULL, 0);

    return bitmap_get_bpp_of_format((bitmap_format_t)(bitmap->format));
}


ret_t bitmap_set_line_length(bitmap_t* bitmap, uint32_t line_length) 
{
    return_value_if_fail(bitmap != NULL, RET_BAD_PARAMS);

    if (bitmap->format == BITMAP_FMT_MONO) 
    {
        bitmap->line_length = BITMAP_MONO_LINE_LENGTH(bitmap->w);
    } 
    else 
    {
        uint32_t bpp = bitmap_get_bpp(bitmap);
        bitmap->line_length = max(bitmap->w * bpp, line_length);
    }

    return RET_OK;
}

uint32_t bitmap_get_line_length(bitmap_t* bitmap) 
{
    return_value_if_fail(bitmap != NULL, 0);

    if (bitmap->line_length == 0) 
    {
        bitmap_set_line_length(bitmap, 0);
    }

    return bitmap->line_length;
}

uint32_t bitmap_get_width(bitmap_t* bitmap) 
{
    return_value_if_fail(bitmap != NULL, 0);
 
    return bitmap->w;
}

uint32_t bitmap_get_height(bitmap_t* bitmap) 
{
    return_value_if_fail(bitmap != NULL, 0);
   
    return bitmap->h;
}

uint32_t bitmap_get_mem_size(bitmap_t* bitmap)
{
    return_value_if_fail(bitmap != NULL, 0);

    return bitmap->w * bitmap->h * bitmap_get_bpp(bitmap);
}

ret_t bitmap_alloc_data(bitmap_t* bitmap) 
{
    return_value_if_fail(bitmap != NULL && bitmap->w > 0 && bitmap->h > 0, RET_BAD_PARAMS);
    uint32_t size = 0;
  
    size = bitmap->line_length * bitmap->h;
    size = ROUND_TO(size, BITMAP_ALIGN_SIZE) + BITMAP_ALIGN_SIZE;

    bitmap->buffer = (uint8_t*)calloc(1, size);
    return_value_if_fail(bitmap->buffer != NULL, RET_OOM);
    bitmap->should_free_data = TRUE;
    bitmap->size = size;

    return RET_OK;
}

ret_t bitmap_init(bitmap_t* bitmap, uint32_t w, uint32_t h, bitmap_format_t format, uint8_t* data) 
{
    return_value_if_fail(bitmap != NULL, RET_BAD_PARAMS);
    uint32_t line_length = 0;
    uint32_t bpp = bitmap_get_bpp_of_format(format);
    
    if (bitmap->format == BITMAP_FMT_MONO) 
    {
        line_length = BITMAP_MONO_LINE_LENGTH(w);
    } 
    else 
    {
        uint32_t bpp = bitmap_get_bpp_of_format(format);
        line_length = max(w * bpp, line_length);
    }

    /* 20230414, 使用共享内存初始化, 有些数据不能清除 */
    if (data == NULL)
    {
        memset(bitmap, 0x00, sizeof(bitmap_t));
    }

    bitmap->w = w;
    bitmap->h = h;
    bitmap->format = format;
    bitmap->line_length = line_length;
    bitmap->bpp = bpp;
    if (bpp < 4) 
    {
        bitmap->flags = BITMAP_FLAG_OPAQUE;
    }

    if (data == NULL) 
    {
        bitmap_alloc_data(bitmap);
    } 
    else 
    {
        bitmap->buffer = data;
    }
    
    return bitmap->buffer != NULL ? RET_OK : RET_OOM;
}

void bitmap_printf(bitmap_t* bitmap)
{
    bitmap_t *p = bitmap;

    log_info("- bitmap info: ----------\n");
    log_info("-- w h stride bpp: %d %d %d %d\n", p->w, p->h, p->line_length, p->bpp);
    log_info("-- flag fmt: %d %d\n", p->flags, p->format);
    log_info("-- name: %s\n", p->name);
    log_info("-- buf size: %p %d\n", p->buffer, p->size);
    log_info("-- shm_name: %s\n", p->shm_name);
    log_info("-- r: %d\n", p->r);
    log_info("-- free hdl date: %d %d\n", p->should_free_handle, p->should_free_data);
    log_info("--------------------------\n");
}

/* bitmap buffer alloc，一般用于队列 */
pthread_mutex_t ref_mutex = PTHREAD_MUTEX_INITIALIZER;


static bitmap_buf_set_t *bitmap_buf_set_new(uint32_t nbufs)
{
    bitmap_buf_set_t *buf_set;

    buf_set = malloc(sizeof(*buf_set));
    if (!buf_set)
    {
        return NULL;
    }
    
    buf_set->next  = 0;
    buf_set->nbufs = nbufs;
    buf_set->bufs  = calloc(nbufs, sizeof(*buf_set->bufs));
    if (!buf_set->bufs)
    {
        free(buf_set);
        return NULL;
    }

    return buf_set;
}

void bitmap_buf_set_destroy(bitmap_buf_set_t *buf_set)
{
    uint32_t i;
    bitmap_t *bitmap;

    if (!buf_set)
    {
        return;
    }

    for (i = 0; i < buf_set->nbufs; i++) 
    {
        bitmap = &buf_set->bufs[i];
        bitmap_destroy(bitmap);
    }

    free(buf_set->bufs);
    free(buf_set);
    buf_set = NULL;
}

bitmap_buf_set_t *bitmap_buf_set_create(uint32_t w, uint32_t h, bitmap_format_t format, uint32_t nbufs) 
{
    bitmap_buf_set_t *buf_set;
    bitmap_t *bitmap;
    uint32_t i;
    ret_t ret = RET_FAIL;

    buf_set = bitmap_buf_set_new(nbufs);
    if (!buf_set)
    {
        goto done;
    }

    for (i = 0; i < nbufs; i++) 
    {
        bitmap = &buf_set->bufs[i];
        ret = bitmap_init(bitmap, w, h, format, NULL);      /* NULL, 由bitmap_init()分配内存 */
        if (RET_OK != ret)
        {
            goto done;
            break;
        }
    }

done:
    if (RET_OK != ret)
    {
        bitmap_buf_set_destroy(buf_set);
    }

    return buf_set;
}

/* 获取一个bitmap buf */
bitmap_t *bitmap_buf_obtain(bitmap_buf_set_t *buf_set)
{
    bitmap_t *bitmap;
    int idx;

    idx = buf_set->next;
    bitmap = &buf_set->bufs[idx];
    if (bitmap->flags & BITMAP_FLAG_USED)
    {
        return NULL;
    }
    bitmap->ref_count = 0;
    bitmap->flags |= BITMAP_FLAG_USED;

    buf_set->next = (idx + 1) % buf_set->nbufs;
    return bitmap;
}

void bitmap_buf_ref(bitmap_t *bitmap)
{
    pthread_mutex_lock(&ref_mutex);
    bitmap->ref_count++;
    pthread_mutex_unlock(&ref_mutex);   
}

void bitmap_buf_unref(bitmap_t *bitmap)
{
    pthread_mutex_lock(&ref_mutex);
    bitmap->ref_count--;
    pthread_mutex_unlock(&ref_mutex);   
}

void bitmap_buf_release(bitmap_t *bitmap)
{
    bitmap_buf_unref(bitmap);
    if (bitmap->ref_count <= 0)
    {
        bitmap->flags &= ~BITMAP_FLAG_USED;
    }
}

