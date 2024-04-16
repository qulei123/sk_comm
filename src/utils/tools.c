
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netdb.h>

#include <string.h>
#include <stdlib.h>
#include "tools.h"


/* copies at most n-1 bytes to dest, always adds a  terminating  null  byte */
char *strlcpy(char *dest, const char *src, size_t n)
{
    if (n > 0)
    {
        strncpy(dest, src, n - 1);
        dest[n - 1] = '\0';
    }

    return dest;
}

/* 获取第一个ip地址 */
int sys_get_eth0_ip(char *buf, int len)
{
    char ip_buf[64];
    struct ifaddrs *ifa, *ifap;   
    int ret, family, addrlen;

    if (getifaddrs(&ifa) != 0)
    {
        perror("getifaddrs"); 
        return -1;
    }
    
    for (ifap = ifa; ifap != NULL; ifap = ifap->ifa_next)
    {
        /* Skip interfaces that have no configured addresses */
        if (ifap->ifa_addr == NULL)
            continue;
        
        /* Skip the loopback interface */
        if (ifap->ifa_flags & IFF_LOOPBACK)
            continue;
        
        /* Skip interfaces that are not UP */
        if (!(ifap->ifa_flags & IFF_UP))
            continue;

        /* Only handle IPv4 and IPv6 addresses */
        family = ifap->ifa_addr->sa_family;
        if (family != AF_INET && family != AF_INET6)
            continue;

        addrlen = (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

        /* Skip IPv6 link-local addresses */
        if (family == AF_INET6)
        {
            struct sockaddr_in6 *sin6;

            sin6 = (struct sockaddr_in6 *)ifap->ifa_addr;
            if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr) || 
                IN6_IS_ADDR_MC_LINKLOCAL(&sin6->sin6_addr))
                continue;
        }

        ret = getnameinfo(ifap->ifa_addr, addrlen, ip_buf, sizeof(ip_buf), NULL, 0, NI_NUMERICHOST);  /* inet_ntop() */
        if (ret != 0) 
        {
            perror("getnameinfo");
        }

        if (!strcmp(ifap->ifa_name, "eth0"))
        {
            memcpy(buf, ip_buf, len);
            return 0;
        }
        //snprintf(buf + strlen(buf), len - strlen(buf), "%s: %s \n", ifap->ifa_name, ip_buf);    /* 自动加'\0' */
    }
    
    freeifaddrs(ifa);
    
    return -1;
}

uint16_t* memset16(uint16_t* buff, uint16_t val, uint32_t size) 
{
    return_value_if_fail(buff != NULL, NULL);
    uint32_t n = 0;
    uint16_t* p = buff;
    uint8_t* pb = (uint8_t*)buff;

    while ((size_t)pb % 4 != 0 && size > 0) 
    {
        *p = val;

        p++;
        size--;
        pb += 2;
    }

    n = size / 8; /*16bytes*/
    if (n > 0) 
    {
        uint32_t* p32 = NULL;
        uint32_t data = val | (val << 16);

        while (n > 0) 
        {
            p32 = (uint32_t*)pb;

            p32[0] = data;
            p32[1] = data;
            p32[2] = data;
            p32[3] = data;

            n--;
            pb += 16;
        }
    }

    n = size % 8;
    if (n > 0) 
    {
        p = (uint16_t*)pb;
        while (n > 0) 
        {
            *p = val;
            p++;
            n--;
        }
    }

    return buff;
}

uint32_t* memset24(uint32_t* buff, void* val, uint32_t size) 
{
    return_value_if_fail(buff != NULL && val != NULL, NULL);
    uint32_t n = 0;
    uint32_t bytes = size * 3;
    uint8_t* pb = (uint8_t*)buff;
    uint8_t* src = (uint8_t*)val;

    while ((size_t)pb % 4 != 0 && size > 0) 
    {
        pb[0] = src[0];
        pb[1] = src[1];
        pb[2] = src[2];

        pb += 3;
        size--;
    }

    bytes = size * 3;
    n = bytes / 12;

    if (n > 0) 
    {
        uint32_t* p = NULL;
        uint32_t data0 = src[0] | src[1] << 8 | src[2] << 16 | src[0] << 24;
        uint32_t data1 = src[1] | src[2] << 8 | src[0] << 16 | src[1] << 24;
        uint32_t data2 = src[2] | src[0] << 8 | src[1] << 16 | src[2] << 24;

        while (n > 0) 
        {
            p = (uint32_t*)pb;
            p[0] = data0;
            p[1] = data1;
            p[2] = data2;
            pb += 12;
            n--;
        }
    }

    bytes = bytes % 12;
    while (bytes > 0) 
    {
        pb[0] = src[0];
        pb[1] = src[1];
        pb[2] = src[2];
        pb += 3;
        bytes -= 3;
    }

    return buff;
}

uint32_t* memset32(uint32_t* buff, uint32_t val, uint32_t size) 
{
    return_value_if_fail(buff != NULL, NULL);
    uint32_t* p = buff;

    while (size-- > 0) 
    {
        *p++ = val;
    }

    return buff;
}

uint16_t* memcpy16(uint16_t* dst, uint16_t* src, uint32_t size) 
{
    return_value_if_fail(dst != NULL && src != NULL, NULL);
    uint16_t* d = dst;
    uint16_t* s = src;

    while (size-- > 0) 
    {
        *d++ = *s++;
    }

    return dst;
}

uint32_t* memcpy32(uint32_t* dst, uint32_t* src, uint32_t size) 
{
    return_value_if_fail(dst != NULL && src != NULL, NULL);
    uint32_t* d = dst;
    uint32_t* s = src;

    while (size-- > 0) 
    {
        *d++ = *s++;
    }

    return dst;
}

void* pixel_copy(void* dst, const void* src, uint32_t size, uint8_t bpp) 
{
    return_value_if_fail(dst != NULL && src != NULL, NULL);

    if (bpp == 2) 
    {
        memcpy16((uint16_t*)dst, (uint16_t*)src, size);
    } 
    else if (bpp == 4) 
    {
        memcpy32((uint32_t*)dst, (uint32_t*)src, size);
    } 
    else
    {
        memcpy(dst, src, size * bpp);
    }

    return dst;
}

static void quick_sort_impl(void** array, size_t left, size_t right, compare_t cmp)
{
    size_t save_left = left;
    size_t save_right = right;
    void* x = array[left];

    while (left < right) 
    {
        while (cmp(array[right], x) >= 0 && left < right) 
        {
            right--;
        }
        
        if (left != right) 
        {
            array[left] = array[right];
            left++;
        }

        while (cmp(array[left], x) <= 0 && left < right)
        {
            left++;
        }
        
        if (left != right) 
        {
            array[right] = array[left];
            right--;
        }
    }
    array[left] = x;

    if (save_left < left) 
    {
        quick_sort_impl(array, save_left, left - 1, cmp);
    }

    if (save_right > left) 
    {
        quick_sort_impl(array, left + 1, save_right, cmp);
    }

    return;
}

ret_t quick_sort(void** array, size_t nr, compare_t cmp) 
{
    ret_t ret = RET_OK;

    return_value_if_fail(array != NULL && cmp != NULL, RET_BAD_PARAMS);

    if (nr > 1) 
    {
        quick_sort_impl(array, 0, nr - 1, cmp);
    }

    return ret;
}


int32_t pointer_to_int(const void* p)
{
    return (char*)p - (char*)(NULL);
}

