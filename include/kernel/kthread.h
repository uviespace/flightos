/**
 * @file include/kernel/kthread.h
 */


#ifndef _KERNEL_KTHREAD_H_
#define _KERNEL_KTHREAD_H_


#include <stdarg.h>
#include <list.h>
#include <asm-generic/thread.h>


#define KTHREAD_CPU_AFFINITY_NONE	(-1)



/* task states */

#define TASK_RUNNING	0xcafe
#define TASK_PARKED	0x0001
#define TASK_NEW	0x0002
#define TASK_DEAD	0x0004


struct task_struct {

	struct thread_info thread_info;


	/* -1 unrunnable, 0 runnable, >0 stopped: */
	volatile long			state;

	void				*stack;
	void				*stack_top;
	void				*stack_bottom;

	int				on_cpu;
	int (*thread_fn)(void *data);
	void *data;


	/* XXX
	 * We can use a guard pattern in a canary, so we can detect if the stack
	 * was corrupted. Since we do not need security, just safety, this
	 * can be any kind of pattern TBD
	 */
	unsigned long stack_canary;


	/* Tasks may have a parent and any number of siblings or children.
	 * If the parent is killed or terminated, so are all siblings and
	 * children.
	 */
	struct task_struct		*parent;
	struct list_head		node;
	struct list_head		siblings;
	struct list_head		children;



};

struct task_struct *kthread_create(int (*thread_fn)(void *data),
				   void *data, int cpu,
				   const char *namefmt,
				   ...);

struct task_struct *kthread_init_main(void);
void kthread_wake_up(struct task_struct *task);
/* XXX dummy */
void switch_to(struct task_struct *next);
void schedule(void);



#endif /* _KERNEL_KTHREAD_H_ */
