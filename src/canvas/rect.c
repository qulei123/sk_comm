﻿
#include <stdlib.h>
#include "tools.h"
#include "rect.h"

#define rectX_fix                \
  {                              \
    if (r->x < 0) {              \
      r->x = 0;                  \
    }                            \
                                 \
    if (r->x > max_w) {          \
      r->x = max_w;              \
      r->w = 0;                  \
    }                            \
                                 \
    if (r->y < 0) {              \
      r->y = 0;                  \
    }                            \
                                 \
    if (r->y > max_h) {          \
      r->y = max_h;              \
      r->h = 0;                  \
    }                            \
                                 \
    if (r->w < 0) {              \
      r->w = 0;                  \
    }                            \
                                 \
    if (r->h < 0) {              \
      r->h = 0;                  \
    }                            \
                                 \
    if ((r->x + r->w) > max_w) { \
      r->w = max_w - r->x;       \
    }                            \
                                 \
    if ((r->y + r->h) > max_h) { \
      r->h = max_h - r->y;       \
    }                            \
                                 \
    if (r->w < 0) {              \
      r->w = 0;                  \
    }                            \
                                 \
    if (r->h < 0) {              \
      r->h = 0;                  \
    }                            \
  }

ret_t rect_merge(rect_t* dr, const rect_t* r) 
{
    return_value_if_fail(r != NULL && dr != NULL, RET_BAD_PARAMS);

    if (r->w > 0 && r->h > 0) 
    {
        if (dr->w > 0 && dr->h > 0) 
        {
            xy_t x = min(dr->x, r->x);
            xy_t y = min(dr->y, r->y);
            wh_t right = max((r->x + r->w), (dr->x + dr->w));
            wh_t bottom = max((r->y + r->h), (dr->y + dr->h));

            dr->x = x;
            dr->y = y;
            dr->w = right - x;
            dr->h = bottom - y;
        } 
        else 
        {
            *dr = *r;
        }
    }

    return RET_OK;
}

bool_t rect_contains(const rect_t* r, xy_t x, xy_t y) 
{
    return_value_if_fail(r != NULL, FALSE);

    return (x >= r->x && x < (r->x + r->w)) && (y >= r->y && y < (r->y + r->h));
}

bool_t rect_has_intersect(const rect_t* r1, const rect_t* r2) 
{
    return_value_if_fail(r1 != NULL && r2 != NULL, FALSE);
    xy_t right1  = 0;
    xy_t right2  = 0;
    xy_t bottom1 = 0;
    xy_t bottom2 = 0;

    right1  = r1->x + r1->w - 1;
    right2  = r2->x + r2->w - 1;
    bottom1 = r1->y + r1->h - 1;
    bottom2 = r2->y + r2->h - 1;

    if (right1 < r2->x || right2 < r1->x || bottom1 < r2->y || bottom2 < r1->y) 
    {
        return FALSE;
    }

    return TRUE;
}

rectf_t rectf_init(float x, float y, float w, float h) 
{
    rectf_t r = {x, y, w, h};
    return r;
}

rect_t rect_init(xy_t x, xy_t y, wh_t w, wh_t h) 
{
    rect_t r = {x, y, w, h};
    return r;
}

rect_t rect_intersect(const rect_t* r1, const rect_t* r2) 
{
    int32_t top = 0;
    int32_t left = 0;
    int32_t bottom = 0;
    int32_t right = 0;
    int32_t bottom1 = 0;
    int32_t right1 = 0;
    int32_t bottom2 = 0;
    int32_t right2 = 0;
    rect_t r = rect_init(0, 0, 0, 0);

    return_value_if_fail(r1 != NULL && r2 != NULL, r);

    bottom1 = r1->y + r1->h - 1;
    bottom2 = r2->y + r2->h - 1;
    right1 = r1->x + r1->w - 1;
    right2 = r2->x + r2->w - 1;

    top = max(r1->y, r2->y);
    left = max(r1->x, r2->x);
    right = min(right1, right2);
    bottom = min(bottom1, bottom2);

    r.x = left;
    r.y = top;
    r.w = right >= left ? (right - left + 1) : 0;
    r.h = bottom >= top ? (bottom - top + 1) : 0;

    return r;
}

rect_t rect_fix(rect_t* r, wh_t max_w, wh_t max_h) 
{
    rectX_fix;
    return *r;
}

rect_t* rect_create(xy_t x, xy_t y, wh_t w, wh_t h) 
{
    rect_t* r = calloc(1, sizeof(rect_t));
    return_value_if_fail(r != NULL, NULL);

    *r = rect_init(x, y, w, h);

    return r;
}

rect_t* rect_set(rect_t* r, xy_t x, xy_t y, wh_t w, wh_t h) 
{
    return_value_if_fail(r != NULL, NULL);

    *r = rect_init(x, y, w, h);

    return r;
}

rect_t* rect_cast(rect_t* rect) 
{
    return_value_if_fail(rect != NULL, NULL);

    return rect;
}

ret_t rect_destroy(rect_t* r) 
{
    return_value_if_fail(r != NULL, RET_BAD_PARAMS);

    free(r);
    r = NULL;

    return RET_OK;
}

rect_t* rect_scale(rect_t* r, float_t scale) 
{
    return_value_if_fail(r != NULL, r);

    if (scale != 1.0f) 
    {
        r->x = roundi(r->x * scale);
        r->y = roundi(r->y * scale);
        r->w = roundi(r->w * scale);
        r->h = roundi(r->h * scale);
    }

    return r;
}

rectf_t* rectf_scale(rectf_t* r, float_t scale) 
{
    return_value_if_fail(r != NULL, r);

    if (scale != 1.0f) 
    {
        r->x = r->x * scale;
        r->y = r->y * scale;
        r->w = r->w * scale;
        r->h = r->h * scale;
    }

    return r;
}

rectf_t rectf_fix(rectf_t* r, wh_t max_w, wh_t max_h) 
{
    rectX_fix;
    return *r;
}

rectf_t rect_to_rectf(const rect_t* r) 
{
    rectf_t tmp_r = {0};
    return_value_if_fail(r != NULL, tmp_r);
    
    tmp_r.x = (float_t)(r->x);
    tmp_r.y = (float_t)(r->y);
    tmp_r.w = (float_t)(r->w);
    tmp_r.h = (float_t)(r->h);

    return tmp_r;
}

rect_t rect_from_rectf(const rectf_t* r) 
{
    rect_t tmp_r = {0};
    return_value_if_fail(r != NULL, tmp_r);
    
    tmp_r.x = roundi(r->x);
    tmp_r.y = roundi(r->y);
    tmp_r.w = roundi(r->w);
    tmp_r.h = roundi(r->h);

    return tmp_r;
}

pointf_t pointf_init(float_t x, float_t y) 
{
    pointf_t p = {x, y};
    return p;
}

point_t point_init(xy_t x, xy_t y) 
{
    point_t p = {x, y};
    return p;
}

void print_rect(const char *tag, rect_t *r) 
{
    printf("%s: xy:%d %d, wh:%d %d\n", tag, r->x, r->y, r->w, r->h);
}

void print_rectf(const char *tag, rectf_t *r) 
{
    printf("%s: xy:%f %f, wh:%f %f\n", tag, r->x, r->y, r->w, r->h);
}

