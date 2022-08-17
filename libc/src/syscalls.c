#include <asm-generic/unistd.h>
#include <syscall.h>

#include <syscalls.h>



__attribute__((noinline))
int sys_read(int fd, void *buf, size_t count)
{
	return SYSCALL3(__NR_write, fd, buf, count);
}


__attribute__((noinline))
int sys_write(int fd, void *buf, size_t count)
{
	return SYSCALL3(__NR_write, fd, buf, count);
}


__attribute__((noinline))
int sys_alloc(size_t size, void **p)
{
	return SYSCALL2(__NR_alloc, size, p);
}


__attribute__((noinline))
int sys_free(void *p)
{
	return SYSCALL1(__NR_free, p);
}


__attribute__((noinline))
int sys_clock_gettime(struct timespec *tp)
{
	return SYSCALL1(__NR_gettime, tp);
}


__attribute__((noinline))
int sys_nanosleep(int flags, const struct timespec *rqtp)
{
	return SYSCALL2(__NR_nanosleep, flags, rqtp);
}
