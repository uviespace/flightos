#ifndef GLUE_H
#define GLUE_H


/*
 * prototypes and declarations needed to build this demo
 */

#define module_init(initfunc)                                   \
        int _module_init(void) __attribute__((alias(#initfunc)));

#define module_exit(exitfunc)					\
        int _module_exit(void) __attribute__((alias(#exitfunc)));

#define KTHREAD_CPU_AFFINITY_NONE	(-1)

int printk(const char *format, ...);

struct task_struct *kthread_create(int (*thread_fn)(void *data),
				   void *data, int cpu,
				   const char *namefmt,
				   ...);
void kthread_wake_up(struct task_struct *task);

void sched_yield(void);



#endif /* GLUE_H */
