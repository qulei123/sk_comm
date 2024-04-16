/*
 * @Author: jiejie
 * @Github: https://github.com/jiejieTop
 * @Date: 2019-12-15 18:27:19
 * @LastEditTime: 2020-02-23 15:01:06
 * @Description: the code belongs to jiejie, please keep the author information and source code according to the license.
 */
#include "platform_mutex.h"
#include <stdio.h>
#include <string.h>

int platform_mutex_init(platform_mutex_t* m, int kind)
{
    pthread_mutexattr_t attr;
    int ret;

    ret = pthread_mutexattr_init(&attr);
    ret |= pthread_mutexattr_settype(&attr, kind);
    ret |= pthread_mutex_init(&(m->mutex), &attr);
    ret |= pthread_mutexattr_destroy(&attr);

    return ret;
}

int platform_mutex_lock(platform_mutex_t* m)
{
#if 0
    int ret;

    ret = pthread_mutex_lock(&(m->mutex));
    if (ret != 0)
    {
        printf("Mutex lock failed: %s\n", strerror(ret));
    }

    return ret;
#else
    return pthread_mutex_lock(&(m->mutex));
#endif
}

int platform_mutex_trylock(platform_mutex_t* m)
{
    return pthread_mutex_trylock(&(m->mutex));
}

int platform_mutex_unlock(platform_mutex_t* m)
{
    return pthread_mutex_unlock(&(m->mutex));
}

int platform_mutex_destroy(platform_mutex_t* m)
{
    return pthread_mutex_destroy(&(m->mutex));
}
