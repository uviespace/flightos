#ifdef DPU_TARGET__GLUE
#ifndef __GLUE_H__
#define __GLUE_H__


#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <byteorder.h>

#ifndef __clockid_t_defined
typedef unsigned long clockid_t;
#define __clockid_t_defined
#endif

#define CLOCK_MONOTONIC		1


/* libc prototypes */
#if !defined(_POSIX_TIMERS)
int clock_gettime(clockid_t clk_id, struct timespec *tp);
#endif /* !defined(_POSIX_TIMERS) */

#if !defined(_POSIX_CLOCK_SELECTION)
int clock_nanosleep(clockid_t clock_id, int flags,
		    const struct timespec *rqtp, struct timespec *rmtp);
#endif /* !defined(_POSIX_CLOCK_SELECTION) */

void perror(const char *s);

void free(void *ptr);
void *malloc(size_t size);
int vprintf(const char *format, va_list ap);
int printf(const char *fmt, ...);


/** AARRRGHHH!!!! */
int32_t __bswapsi2(int32_t a);




/* external (leanos) function prototypes */

#define module_init(initfunc)                                   \
        int _module_init(void) __attribute__((alias(#initfunc)));

#define module_exit(exitfunc)                                   \
        int _module_exit(void) __attribute__((alias(#exitfunc)));

#define KTHREAD_CPU_AFFINITY_NONE       (-1)

struct timespec get_ktime(void);

int printk(const char *format, ...);

struct task_struct *kthread_create(int (*thread_fn)(void *data),
                                   void *data, int cpu,
                                   const char *namefmt,
                                   ...);
void kthread_wake_up(struct task_struct *task);

void sched_yield(void);

void kfree(void *ptr);
void *kmalloc(size_t size);



#endif /* __GLUE_H__ */
#endif /* DPU_TARGET__GLUE */
