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
#include <kernel/signals.h>

#include <compiler.h>
#include <generated/autoconf.h>

compile_time_assert(!(CONFIG_STACK_SIZE & STACK_ALIGN), STACK_SIZE_UNALIGNED);

extern struct thread_info *current_set[];


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

/* maximum length of task name string */
#define TASK_NAME_LEN	64


/* XXX TODO
 * so in order to efficiently emit signals (especially when they are emitted
 * by some ISR), we actually want a pre-reserved stack frame and the
 * actual function which executes the signal handler to be prepared the moment
 * a task registers itself to any signal.
 *
 * I think we need the following prerequisites:
 *
 * - from task_struct, extract thread_info, and the stack pointers
 *   into a struct task_core
 *
 * - we make the current thread_info a pointer and have it point to
 *   thread_actual by default
 *
 * - we add a pointer task_core *thread_sigstack, which we allocate
 *   when a signal is connected
 *
 * - we split the stack allocation and architecture-relevant sections of
 *   kthread_create_internal out to kthread_create_core; this way we
 *   can add a signal stack using something like kthread_sigstack_create()
 *
 * - since the pending signal counter increments when a signal is added,
 *   we only need to check for != 0 in the corresponding scheduler
 *   and switch the thread_inf pointer to the task_core signal subtask, since all
 *   other scheduling-relevant thread parameters remain the same as we only
 *   need to switch code paths intermittently and also want to preserve
 *   the original schedule
 *
 * - the signal wrapper will run in a loop which decrements the pending
 *   signal counter when a queued handler has been executed and
 *   re-sets the thread_info pointer to the task_core of thread_actual,
 *   then calls sched_yield()
 *
 * - we will have to block ksigaction() for tasks which run in signal handler
 *   mode, otherwise somebody will get funny ideas on connecting signals
 *   to signal handlers...
 *
 *
 * - we should eventually do some cleanup and remove the signal subtask
 *   when a task disconnects all signals; in any case, we will deallocate
 *   it during kthread_free()
 *
 */


struct task_core {

	struct thread_info		thread_info;

	void				*stack;
	void				*stack_top;
	void				*stack_bottom;

	int				(*thread_fn)(void *data);
	void				*data;
};



struct task_struct {

	struct task_core		*active;
	struct task_core		 tsk;
	struct task_core		*sig;


	/* -1 unrunnable, 0 runnable, >0 stopped: */
	volatile long			state;

	int				on_cpu;
	char				*name;


	/* XXX
	 * We can use a guard pattern in a canary, so we can detect if the stack
	 * was corrupted. Since we do not need security, just safety, this
	 * can be any kind of pattern TBD
	 */
	unsigned long stack_canary;


	struct scheduler		*sched;

	struct sched_attr		attr;


	/* XXX TODO: check whether this bug persists in more recent versions
	 * bcc is SO extremely stupid, it misaligns 64-bit types and generates
	 * ldd's to laod them; also, it does not respect the aligned attribute
	 */
	int				unused;

	ktime				runtime; /* remaining runtime in this period  */
	ktime				wakeup; /* start of next period */
	ktime				deadline; /* deadline of current period */

	ktime				create; /* start of next period */

	ktime				wakeup_first;
	ktime				exec_start;
	ktime				exec_stop;
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

	struct list_head		ksig_node;
	struct list_head		ksig_queue;
	struct list_head		ksig_handlers;
	int				signal;


}  __attribute__ ((aligned (8)));

struct task_struct *kthread_create(int (*thread_fn)(void *data),
				   void *data, int cpu,
				   const char *namefmt,
				   ...);

struct task_struct *kthread_init_main(void);
int kthread_wake_up(struct task_struct *task);

void kthread_free(struct task_struct *task);

int kthread_set_sched_edf(struct task_struct *task, unsigned long period_us,
			   unsigned long deadline_rel_us, unsigned long wcet_us);

int kthread_set_sched_rr(struct task_struct *task, unsigned long priority);

ktime kthread_get_total_runtime(void);
void kthread_clear_total_runtime(void);

int kthread_sigstack_create(struct task_struct *task,
			   int (*thread_fn)(void *data));

#endif /* _KERNEL_KTHREAD_H_ */
