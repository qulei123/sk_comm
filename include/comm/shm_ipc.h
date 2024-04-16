
#ifndef __SHM_IPC_H__
#define __SHM_IPC_H__


void *shm_ipc_create(const char *name, unsigned int size);
void *shm_ipc_open(const char *name, unsigned int size);
int shm_ipc_close(const char *name, void *addr, unsigned int size);


#endif /* __SHM_IPC_H__ */

