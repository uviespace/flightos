/**
 * @file include/kernel/kthread.h
 */


#ifndef _KERNEL_KTHREAD_H_
#define _KERNEL_KTHREAD_H_


#include <stdarg.h>
#include <list.h>
#include <asm-generic/thread.h>
#include <kernel/time.h>
#include <kernel/sched.h>

#include <compiler.h>
#include <generated/autoconf.h>

compile_time_assert(!(CONFIG_STACK_SIZE & STACK_ALIGN), STACK_SIZE_UNALIGNED);


#define KTHREAD_CPU_AFFINITY_NONE	(-1)



/* task states */

#define TASK_RUN	0x0000
#define TASK_IDLE	0x0001
#define TASK_NEW	0x0002
#define TASK_DEAD	0x0004
#define TASK_BUSY	0x0005

/* task flags */
#define TASK_RUN_ONCE	(1 << 0)	/* execute for only one time slice */
#define TASK_NO_CLEAN	(1 << 30)	/* user takes care of cleanup */
#define TASK_NO_CHECK	(1 << 31)	/* skip any validation checks */



//extern volatile int sched_edf;
struct task_struct {

	struct thread_info thread_info;


	/* -1 unrunnable, 0 runnable, >0 stopped: */
	volatile long			state;

	void				*stack;
	void				*stack_top;
	void				*stack_bottom;

	int				on_cpu;
	int				(*thread_fn)(void *data);
	void				*data;
	char				*name;


	/* XXX
	 * We can use a guard pattern in a canary, so we can detect if the stack
	 * was corrupted. Since we do not need security, just safety, this
	 * can be any kind of pattern TBD
	 */
	unsigned long stack_canary;


	struct scheduler		*sched;

	struct sched_attr		attr;


	/* bcc is SO extremely stupid, it misaligns 64-bit types and generates
	 * ldd's to laod them; also, it does not respect the aligned attribute
	 */
	int				unused;

	ktime				runtime; /* remaining runtime in this period  */
	ktime				wakeup; /* start of next period */
	ktime				deadline; /* deadline of current period */

	ktime				exec_start;
	ktime				total;
	unsigned long			slices;

	unsigned long			flags;


	/* Tasks may have a parent and any number of siblings or children.
	 * If the parent is killed or terminated, so are all siblings and
	 * children.
	 */
	struct task_struct		*parent;
	struct list_head		node;
	struct list_head		siblings;
	struct list_head		children;



}  __attribute__ ((aligned (8)));

struct task_struct *kthread_create(int (*thread_fn)(void *data),
				   void *data, int cpu,
				   const char *namefmt,
				   ...);

struct task_struct *kthread_init_main(void);
int kthread_wake_up(struct task_struct *task);

void kthread_free(struct task_struct *task);

void kthread_set_sched_edf(struct task_struct *task, unsigned long period_us,
			  unsigned long wcet_us, unsigned long deadline_rel_us);

#endif /* _KERNEL_KTHREAD_H_ */
