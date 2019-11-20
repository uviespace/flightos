#ifndef GLUE_H
#define GLUE_H


#include <stddef.h>
#include <stdint.h>

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
int kthread_wake_up(struct task_struct *task);

void sched_yield(void);

int smp_cpu_id(void);

void kthread_set_sched_edf(struct task_struct *task,
			   unsigned long period_us,
			   unsigned long wcet_us,
			   unsigned long deadline_rel_us);


extern struct sysset *sys_set;
struct sysobj *sysset_find_obj(struct sysset *sysset, const char *path);
void sysobj_show_attr(struct sysobj *sobj, const char *name, char *buf);

#endif /* GLUE_H */
