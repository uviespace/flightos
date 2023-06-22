#include <asm-generic/unistd.h>
#include <syscall.h>

#include <syscalls.h>
#include <grspw2.h>



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


/* XXX replace once file subsys is available */
__attribute__((noinline))
int sys_grspw2(const struct grspw2_data *data)
{
	return SYSCALL1(__NR_grspw2, data);
}


__attribute__((noinline))
int sys_thread_create(const thread_t *t)
{
	return SYSCALL1(__NR_thread_create, t);
}


__attribute__((noinline))
int sys_sched_yield(void)
{
	return SYSCALL0(__NR_sched_yield);
}


__attribute__((noinline))
int sys_watchdog(unsigned long timeout_ns, int enable)
{
	return SYSCALL2(__NR_watchdog, timeout_ns, enable);
}


__attribute__((noinline))
int sys_sched_prog_seg(void *addr, int argc, char *argv[])
{
	return SYSCALL3(__NR_sched_prog_seg, addr, argc, argv);
}


__attribute__((noinline))
int sys_sysctl_show_attr(const char *path, const char *name, char *buf)
{
	return SYSCALL3(__NR_sysctl_show_attr, path, name, buf);
}


__attribute__((noinline))
int sysctl_store_attr(const char *path, const char *name, const char *buf, size_t len)
{
	return SYSCALL4(__NR_sysctl_store_attr, path, name, buf, len);
}
