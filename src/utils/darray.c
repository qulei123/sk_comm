
#include <stdlib.h>
#include <string.h>

#include "deftypes.h"
#include "tools.h"
#include "darray.h"


static int32_t darray_bsearch_index_impl(darray_t* darray, compare_t cmp, void* ctx, int32_t* ret_low);

#if 0
static ret_t default_destroy(void* data) 
{
    free(data);

    return RET_OK;
}
#endif

static ret_t dummy_destroy(void* data) 
{
    return RET_OK;
}

static int pointer_compare(const void* a, const void* b) 
{
    return ((const char*)a - (const char*)b);
}

#if 0
static int compare_always_equal(const void* a, const void* b) 
{
    return 0;
}
#endif

darray_t* darray_create(uint32_t capacity, destroy_t destroy, compare_t compare) 
{
    darray_t* darray = calloc(1, sizeof(darray_t));
    return_value_if_fail(darray != NULL, NULL);

    if (darray_init(darray, capacity, destroy, compare)) 
    {
        return darray;
    } 
    else 
    {
        free(darray);
        return NULL;
    }
}

darray_t* darray_init(darray_t* darray, uint32_t capacity, destroy_t destroy, compare_t compare) 
{
    return_value_if_fail(darray != NULL, NULL);

    darray->size     = 0;
    darray->capacity = 0;
    darray->elms     = NULL;
    darray->destroy  = destroy != NULL ? destroy : dummy_destroy;
    darray->compare  = compare != NULL ? compare : pointer_compare;

    if (capacity > 0) 
    {
        darray->elms = calloc(capacity, sizeof(void*));
        return_value_if_fail(darray->elms != NULL, NULL);
        darray->capacity = capacity;
    }

    return darray;
}

static bool_t darray_extend(darray_t* darray) 
{
    if (darray->elms != NULL && darray->size < darray->capacity) 
    {
        return TRUE;
    } 
    else 
    {
        void* elms = NULL;
        uint32_t capacity = (darray->capacity >> 1) + darray->capacity + 1;

        elms = realloc(darray->elms, capacity * sizeof(void*));
        if (elms) 
        {
            darray->elms = elms;
            darray->capacity = capacity;

            return TRUE;
        } 
        else 
        {
            return FALSE;
        }
    }
}

int32_t darray_find_index_ex(darray_t* darray, compare_t compare, void* data) 
{
    return_value_if_fail(darray != NULL, -1);
    int32_t i = 0;
    int32_t size = 0;
    void** elms = NULL;
    
    elms = darray->elms;
    size = darray->size;

    if (compare == NULL) 
    {
        compare = darray->compare ? darray->compare : pointer_compare;
    }

    for (i = 0; i < size; i++) 
    {
        void* iter = elms[i];
        if (compare(iter, data) == 0) 
        {
            return i;
        }
    }

    return -1;
}

int32_t darray_find_index(darray_t* darray, void* data) 
{
    return darray_find_index_ex(darray, NULL, data);
}

ret_t darray_remove_index(darray_t* darray, uint32_t index) 
{
    int32_t i = 0;
    int32_t size = 0;
    void** elms = NULL;
    return_value_if_fail(darray != NULL && index < darray->size, RET_BAD_PARAMS);

    elms = darray->elms;
    darray->destroy(elms[index]);
    elms[index] = NULL;

    for (size = darray->size - 1, i = index; i < size; i++) 
    {
        elms[i] = elms[i + 1];
    }

    elms[i] = NULL;
    darray->size--;

    return RET_OK;
}

 ret_t darray_remove_range(darray_t* darray, uint32_t start, uint32_t end) 
 {
    return_value_if_fail(darray != NULL && start < end && end <= darray->size, RET_BAD_PARAMS);

    if (darray->elms != NULL) 
    {
        uint32_t i = 0, j = 0;
        void** elms = darray->elms;

        for (i = start, j = end; i < darray->size; i++, j++) 
        {
            if (i < end) 
            {
                void* iter = elms[i];
                darray->destroy(iter);
            }
            if (j < darray->size) 
            {
                elms[i] = elms[j];
            } 
            else 
            {
                elms[i] = NULL;
            }
        }
        darray->size -= (end - start);
    }

    return RET_OK;
}

ret_t darray_remove_ex(darray_t* darray, compare_t cmp, void* ctx) 
{
    int32_t index = 0;
    return_value_if_fail(darray != NULL, RET_BAD_PARAMS);

    index = darray_find_index_ex(darray, cmp, ctx);
    if (index < 0) 
    {
        return RET_NOT_FOUND;
    } 
    else 
    {
        return darray_remove_index(darray, index);
    }
}

ret_t darray_remove(darray_t* darray, void* data) 
{
    return darray_remove_ex(darray, NULL, data);
}

ret_t darray_remove_all(darray_t* darray, compare_t cmp, void* ctx) 
{
    int32_t i = 0;
    int32_t k = 0;
    int32_t size = 0;
    void** elms = NULL;
    return_value_if_fail(darray != NULL, RET_BAD_PARAMS);

    elms = darray->elms;
    size = darray->size;
    cmp = cmp != NULL ? cmp : darray->compare;

    for (i = 0, k = 0; i < size; i++) 
    {
        void* iter = elms[i];
        if (cmp(iter, ctx) == 0) 
        {
            darray->destroy(iter);
            elms[i] = NULL;
        } 
        else 
        {
            if (k != i) 
            {
                elms[k] = elms[i];
            }
            k++;
        }
    }
    darray->size = k;

    return RET_OK;
}

ret_t darray_sort(darray_t* darray, compare_t cmp) 
{
    return_value_if_fail(darray != NULL, RET_BAD_PARAMS);
    cmp = cmp != NULL ? cmp : darray->compare;

    return quick_sort(darray->elms, darray->size, cmp);
}

ret_t darray_find_all(darray_t* darray, compare_t cmp, void* ctx, darray_t* matched) 
{
    int32_t i = 0;
    int32_t size = 0;
    void** elms = NULL;
    return_value_if_fail(darray != NULL && matched != NULL, RET_BAD_PARAMS);

    elms = darray->elms;
    size = darray->size;
    cmp = cmp != NULL ? cmp : darray->compare;

    for (i = 0; i < size; i++) 
    {
        void* iter = elms[i];
        if (cmp(iter, ctx) == 0) 
        {
            return_value_if_fail(darray_push(matched, iter) == RET_OK, RET_OOM);
        }
    }

    return RET_OK;
}

void* darray_find_ex(darray_t* darray, compare_t cmp, void* ctx) 
{
    int32_t pos = darray_find_index_ex(darray, cmp, ctx);

    return pos >= 0 ? darray->elms[pos] : NULL;
}

void* darray_find(darray_t* darray, void* data) 
{
    return darray_find_ex(darray, NULL, data);
}

void* darray_pop(darray_t* darray) 
{
    return_value_if_fail(darray != NULL, NULL);

    if (darray->size > 0) 
    {
        return darray->elms[--darray->size];
    } 
    else 
    {
        return NULL;
    }
}

void* darray_tail(darray_t* darray) 
{
    return_value_if_fail(darray != NULL && darray->size > 0, NULL);

    return darray->elms[darray->size - 1];
}

void* darray_head(darray_t* darray) 
{
    return_value_if_fail(darray != NULL && darray->size > 0, NULL);

    return darray->elms[0];
}

/**
 * 插入数据有2中方式：
 * 1. 普通插入：index == darray->size
 * 2. 排序插入：index != darray->size
 */
ret_t darray_insert(darray_t* darray, uint32_t index, void* data) 
{
    return_value_if_fail(darray != NULL, RET_BAD_PARAMS);
    void** s = NULL;
    void** d = NULL;
    void** p = NULL;

    index = min(index, darray->size);
    return_value_if_fail(darray_extend(darray), RET_OOM);

    p = darray->elms + index;
    d = darray->elms + darray->size;
    s = d - 1;
    
    while (s >= p)          /* 应该是排序插入会用到 */
    {
        *d-- = *s--;
    }

    *p = data;
    darray->size++;

    return RET_OK;
}

ret_t darray_sorted_insert(darray_t* darray, void* data, compare_t cmp, bool_t replace_if_exist) 
{
    int32_t low = -1;
    int32_t index = 0;
    return_value_if_fail(darray != NULL, RET_BAD_PARAMS);

    if (darray->size == 0) 
    {
        return darray_push(darray, data);
    }

    index = darray_bsearch_index_impl(darray, cmp, data, &low);
    if (index >= 0) 
    {
        if (replace_if_exist) 
        {
            return darray_replace(darray, index, data);
        }
    } 
    else 
    {
        index = low;
    }

    if (index >= darray->size) 
    {
        return darray_push(darray, data);
    } 
    else 
    {
        return darray_insert(darray, index, data);
    }
}

ret_t darray_push(darray_t* darray, void* data) 
{
    return_value_if_fail(darray != NULL, RET_BAD_PARAMS);

    return darray_insert(darray, darray->size, data);
}

ret_t darray_push_unique(darray_t* darray, void* data)
{
    return_value_if_fail(darray != NULL, RET_BAD_PARAMS);
    if (darray_find_index(darray, data) >= 0) 
    {
        return RET_FAIL;
    }

    return darray_insert(darray, darray->size, data);
}

int32_t darray_count(darray_t* darray, void* data) 
{
    int32_t n = 0;
    return_value_if_fail(darray != NULL, 0);

    if (darray->elms != NULL) 
    {
        uint32_t i = 0;
        void** elms = darray->elms;

        for (i = 0; i < darray->size; i++) 
        {
            void* iter = elms[i];
            if (darray->compare(iter, data) == 0) 
            {
                n++;
            }
        }
    }

    return n;
}

ret_t darray_foreach(darray_t* darray, visit_t visit, void* ctx) 
{
    return_value_if_fail(darray != NULL && visit != NULL, RET_BAD_PARAMS);

    if (darray->elms != NULL) 
    {
        uint32_t i = 0;
        void** elms = darray->elms;

        for (i = 0; i < darray->size; i++) 
        {
            void* iter = elms[i];
            ret_t ret = visit(ctx, iter);
            if (ret != RET_OK) 
            {
                return ret;
            }
        }
    }

    return RET_OK;
}

ret_t darray_clear(darray_t* darray) 
{
    return_value_if_fail(darray != NULL, RET_BAD_PARAMS);

    if (darray->elms != NULL) 
    {
        uint32_t i = 0;
        void** elms = darray->elms;

        for (i = 0; i < darray->size; i++) 
        {
            void* iter = elms[i];
            darray->destroy(iter);
        }

        darray->size = 0;
    }

    return RET_OK;
}

ret_t darray_set(darray_t* darray, uint32_t index, void* data) 
{
    return_value_if_fail(darray != NULL && index < darray->size, RET_BAD_PARAMS);

    darray->elms[index] = data;

    return RET_OK;
}

ret_t darray_replace(darray_t* darray, uint32_t index, void* data) 
{
    return_value_if_fail(darray != NULL && index < darray->size, RET_BAD_PARAMS);

    if (darray->destroy != NULL) 
    {
        darray->destroy(darray->elms[index]);
    }
    darray->elms[index] = data;

    return RET_OK;
}

void* darray_get(darray_t* darray, uint32_t index) 
{
  return_value_if_fail(darray != NULL && index < darray->size, NULL);

  return darray->elms[index];
}

ret_t darray_deinit(darray_t* darray) 
{
    return_value_if_fail(darray != NULL, RET_BAD_PARAMS);

    if (darray->elms != NULL) 
    {
        darray_clear(darray);
        free(darray->elms);
        memset(darray, 0x00, sizeof(darray_t));
    }

    return RET_OK;
}

ret_t darray_destroy(darray_t* darray) 
{
    return_value_if_fail(darray != NULL, RET_BAD_PARAMS);

    darray_deinit(darray);
    free(darray);
    darray = NULL;

    return RET_OK;
}

static int32_t darray_bsearch_index_impl(darray_t* darray, compare_t cmp, void* ctx, int32_t* ret_low) 
{
    int32_t low = 0;
    int32_t mid = 0;
    int32_t high = 0;
    int32_t result = 0;
    void* iter = NULL;
    return_value_if_fail(darray != NULL, -1);

    if (darray->size == 0) 
    {
        return -1;
    }
    if (cmp == NULL) 
    {
        cmp = darray->compare;
    }

    high = darray->size - 1;
    while (low <= high) 
    {
        mid = low + ((high - low) >> 1);
        iter = darray->elms[mid];

        result = cmp(iter, ctx);

        if (result == 0) 
        {
            return mid;
        } 
        else if (result < 0) 
        {
            low = mid + 1;
        } 
        else 
        {
            high = mid - 1;
        }
    }

    if (ret_low != NULL) 
    {
        *ret_low = low;
    }

    return -1;
}

int32_t darray_bsearch_index(darray_t* darray, compare_t cmp, void* ctx) 
{
    return darray_bsearch_index_impl(darray, cmp, ctx, NULL);
}

void* darray_bsearch(darray_t* darray, compare_t cmp, void* ctx) 
{
    int32_t index = darray_bsearch_index(darray, cmp, ctx);
    if (index >= 0) 
    {
        return darray->elms[index];
    } 
    else 
    {
        return NULL;
    }
}

