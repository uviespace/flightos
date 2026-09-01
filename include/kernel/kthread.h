/**
 * @file include/kernel/kthread.h
 * @ingroup schedthread
 * @ingroup threadsys
 *
 * @brief High-level kernel-thread lifecycle, task state, and scheduling interface.
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

/** @brief per-CPU pointer to the currently running thread_info */
extern struct thread_info *current_set[];

/** @brief special value indicating no CPU affinity for a task */
#define KTHREAD_CPU_AFFINITY_NONE	(-1)


/* task states */

/** @brief task is runnable and may be scheduled */
#define TASK_RUN	0x0000
/** @brief task is idle */
#define TASK_IDLE	0x0001
/** @brief task is newly created */
#define TASK_NEW	0x0002
/** @brief task has terminated */
#define TASK_DEAD	0x0004
/** @brief task is busy (waiting for resources) */
#define TASK_BUSY	0x0005

/* task flags */
/** @brief execute task for only one time slice */
#define TASK_RUN_ONCE	(1 << 0)
/** @brief caller takes care of cleanup */
#define TASK_NO_CLEAN	(1 << 30)
/** @brief skip validation checks */
#define TASK_NO_CHECK	(1 << 31)

/** @brief maximum length of a task name string */
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


/**
 * @brief core thread execution context
 *
 * Holds the essential thread state: stack, thread_info, and the
 * thread entry function. A task_struct may have multiple task_core
 * instances (e.g., for signal handler stacks).
 */
struct task_core {

	struct thread_info		thread_info;	/*!< thread info for scheduler */

	void				*stack;		/*!< stack memory base */
	void				*stack_top;	/*!< top of the stack */
	void				*stack_bottom;	/*!< bottom of the stack */

	int				(*thread_fn)(void *data); /*!< thread entry function */
	void				*data;		/*!< argument passed to thread_fn */
};



/**
 * @brief kernel task structure
 *
 * Main control block for a kernel thread. Contains scheduling state,
 * timing information, signal handling, and hierarchical relationships.
 */
struct task_struct {

	struct task_core		*active;	/*!< currently active task_core */
	struct task_core		 tsk;		/*!< primary thread core */
	struct task_core		*sig;		/*!< signal handler task core (or NULL) */

	volatile long			state;		/*!< task state: -1 unrunnable, 0 runnable, >0 stopped */

	int				on_cpu;		/*!< CPU id this task is running on */
	char				*name;		/*!< human-readable task name */

	unsigned long stack_canary; /*!< stack corruption detection pattern */

	struct scheduler		*sched;		/*!< scheduler this task is assigned to */

	struct sched_attr		attr;		/*!< scheduling attributes */

	int				unused;		/*!< padding for alignment (bcc workaround) */

	ktime				runtime;	/*!< remaining runtime in current period */
	ktime				wakeup;		/*!< start of next period */
	ktime				deadline;	/*!< deadline of current period */

	ktime				create;		/*!< time of task creation */

	ktime				wakeup_first;	/*!< time of first wakeup */
	ktime				exec_start;	/*!< start of current execution slice */
	ktime				exec_stop;	/*!< end of current execution slice */
	ktime				total;		/*!< total accumulated runtime */
	unsigned long			slices;		/*!< number of scheduled slices */

	unsigned long			flags;		/*!< task flags (TASK_RUN_ONCE, etc.) */


	struct task_struct		*parent;	/*!< parent task */
	struct list_head		node;		/*!< node in scheduler queue */
	struct list_head		siblings;	/*!< list of sibling tasks */
	struct list_head		children;	/*!< list of child tasks */

	struct list_head		ksig_queue;	/*!< queued signal info */
	struct list_head		ksig_handlers;	/*!< registered signal handlers */
	size_t				sig_cnt;	/*!< number of pending signals */


}  __attribute__ ((aligned (8)));

/**
 * @brief create a new kernel thread
 * @param thread_fn: entry point function for the thread
 * @param data: opaque pointer passed to thread_fn
 * @param cpu: CPU id to bind the thread to, or -1 for no affinity
 * @param namefmt: printf-style format string for the task name
 * @return pointer to the new task_struct, or NULL on failure
 */
struct task_struct *kthread_create(int (*thread_fn)(void *data),
				   void *data, int cpu,
				   const char *namefmt,
				   ...);

/**
 * @brief initialize the main (bootstrap) kernel thread
 * @return pointer to the main task_struct
 */
struct task_struct *kthread_init_main(void);

/**
 * @brief wake up a task and make it runnable
 * @param task: the task to wake up
 * @return 0 on success, negative error code on failure
 */
int kthread_wake_up(struct task_struct *task);

/**
 * @brief free a kernel thread and release its resources
 * @param task: the task to free
 */
void kthread_free(struct task_struct *task);

/**
 * @brief set EDF (Earliest Deadline First) scheduling for a task
 * @param task: the task to configure
 * @param period_us: scheduling period in microseconds
 * @param deadline_rel_us: relative deadline from period start in microseconds
 * @param wcet_us: worst-case execution time in microseconds
 * @return 0 on success, negative error code on failure
 */
int kthread_set_sched_edf(struct task_struct *task, unsigned long period_us,
			   unsigned long deadline_rel_us, unsigned long wcet_us);

/**
 * @brief set Round-Robin scheduling for a task
 * @param task: the task to configure
 * @param priority: static priority level
 * @return 0 on success, negative error code on failure
 */
int kthread_set_sched_rr(struct task_struct *task, unsigned long priority);

/**
 * @brief get the total accumulated runtime of all tasks
 * @return total runtime in nanoseconds since boot
 */
ktime kthread_get_total_runtime(void);

/**
 * @brief reset the total accumulated runtime counter to zero
 */
void kthread_clear_total_runtime(void);

/**
 * @brief create a signal handler stack for a task
 * @param task: the task to add a signal stack to
 * @param thread_fn: signal handler entry point
 * @return 0 on success, negative error code on failure
 */
int kthread_sigstack_create(struct task_struct *task,
			   int (*thread_fn)(void *data));

#endif /* _KERNEL_KTHREAD_H_ */
