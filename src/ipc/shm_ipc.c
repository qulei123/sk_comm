/* 
 * 文件名称：shm_ipc.c
 * 摘     要：进程通信共享内存
 *  
 * 修改历史   版本号         Author   修改内容
 *--------------------------------------------------
 * 2022.09.23   v1      ql     创建文件
 * 
 *--------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "deftypes.h"
#include "shm_ipc.h"


#define SHM_NAME     "/shm_cfg"

void *shm_ipc_create(const char *name, unsigned int size)
{
    int fd, ret;
    const char *shm_name = (NULL != name) ? name : SHM_NAME;
    int flags = O_RDWR | O_CREAT | O_EXCL;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    void *addr;

    //shm_unlink(shm_name);
    fd = shm_open(shm_name, flags, mode);
    if(fd == -1)
    {
        if (EEXIST == errno)
        {
            fd = shm_open(shm_name, O_RDWR, mode);
            if (fd == -1)
            {
                log_eno("shm_open\n");
                return NULL;
            }
        }
        else
        {
            log_eno("shm_open\n");
            return NULL;
        }
    }

    ret = ftruncate(fd, size);
    if(ret == -1)
    {
        log_eno("ftruncate\n");
        return NULL;  
    }
    
    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == addr) 
    {
        log_eno("error map\n");
        return NULL;  
    }

    return addr;  
}

void *shm_ipc_open(const char *name, unsigned int size)
{
    int fd;
    const char *shm_name = (NULL != name) ? name : SHM_NAME;
    int flags = O_RDWR;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    void *addr;

    fd = shm_open(shm_name, flags, mode);
    if(fd == -1)
    {
        log_eno("shm_open\n");
        return NULL;
    }
    
    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == addr)
    {
        log_eno("error map\n");
        return NULL;  
    }

    return addr;
}

int shm_ipc_close(const char *name, void *addr, unsigned int size)
{   
    const char *shm_name = (NULL != name) ? name : SHM_NAME;
    
    if(NULL == addr)
    {
        return 0;  
    }   
    munmap(addr, size);
    shm_unlink(shm_name);
    
    return 0;
}

