#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_


#include <sys/types.h>


int sys_write(int fd, void *buf, size_t count);
int sys_read(int fd, void *buf, size_t count);
int sys_alloc(size_t size, void **p);
int sys_free(void *p);
int sys_clock_gettime(struct timespec *tp);
int sys_nanosleep(int flags, const struct timespec *rqtp);



#if 0
void *sbrk(intptr_t incremen);
#endif

#endif /* _SYSCALLS_H_ */
