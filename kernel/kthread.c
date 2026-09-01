/**
 * @file kernel/kthread.c
 * @ingroup schedthread
 * @ingroup threadsys
 * @defgroup threadsys Kernel Thread Lifecycle
 *
 * @brief Generic kernel-thread creation, startup preparation, wakeup, and cleanup.
 */


#include <kernel/kthread.h>
#include <kernel/export.h>
#include <kernel/smp.h>
#include <kernel/kmem.h>
#include <kernel/err.h>
#include <kernel/printk.h>

#include <asm-generic/irqflags.h>

#include <asm/io.h>
#include <asm/switch_to.h>
#include <asm/spinlock.h>

#include <kernel/string.h>

#include <kernel/tick.h>


#define MSG "KTHREAD: "

static struct spinlock kthread_spinlock;

struct thread_info *current_set[CONFIG_SMP_CPUS_MAX]; /* XXX */


/**
 * @brief get the total runtime of the current thread
 *
 * @note this is done on a best-effort basis without ensuring atomicity
 *
 * @return the total accumulated runtime of the current thread
 */

ktime kthread_get_total_runtime(void)
{
	return current_set[smp_cpu_id()]->task->total;
}


/**
 * @brief clear the total runtime of the current thread
 *
 * @note this is done on a best-effort basis without ensuring atomicity
 */

void kthread_clear_total_runtime(void)
{
	current_set[smp_cpu_id()]->task->total = 0;
}


/**
 * @brief lock critical kthread section
 */

static void kthread_lock(void)
{
	spin_lock_raw(&kthread_spinlock);
}


/**
 * @brief unlock critical kthread section
 */

static void kthread_unlock(void)
{
	spin_unlock(&kthread_spinlock);
}


/**
 * @brief set EDF scheduling parameters for a task
 *
 * Configures a task with Earliest Deadline First scheduling, setting the
 * period, relative deadline, and worst-case execution time.
 *
 * @param task the task to configure
 * @param period_us the scheduling period in microseconds
 * @param deadline_rel_us the relative deadline in microseconds
 * @param wcet_us the worst-case execution time in microseconds
 *
 * @return 0 on success, or a negative error code
 */

int kthread_set_sched_edf(struct task_struct *task, unsigned long period_us,
			   unsigned long deadline_rel_us, unsigned long wcet_us)
{
	struct sched_attr attr;

	sched_get_attr(task, &attr);
	attr.policy       = KSCHED_EDF;
	attr.period       = us_to_ktime(period_us);
	attr.deadline_rel = us_to_ktime(deadline_rel_us);
	attr.wcet         = us_to_ktime(wcet_us);

	return sched_set_attr(task, &attr);
}


/**
 * @brief set Round-Robin scheduling parameters for a task
 *
 * Configures a task with Round-Robin scheduling at the given priority level.
 *
 * @param task the task to configure
 * @param priority the scheduling priority
 *
 * @return 0 on success, or a negative error code
 */

int kthread_set_sched_rr(struct task_struct *task, unsigned long priority)
{
	struct sched_attr attr;

	sched_get_attr(task, &attr);
	attr.policy	  = KSCHED_RR;
	attr.priority     = priority;

	return sched_set_attr(task, &attr);
}


/* we should have a thread with a semaphore which is unlocked by schedule()
 * if dead tasks were added to the "dead" list
 */

/**
 * @brief free a kernel thread and its resources
 *
 * Releases signal handlers, stack, name, and task structure. Tasks with
 * the TASK_NO_CLEAN flag are removed from the task list but their memory
 * is not freed.
 *
 * @param task the task to free
 */

void kthread_free(struct task_struct *task)
{
	if (task->flags & TASK_NO_CLEAN) /* delete from list as well */
		return;


	ksignal_drop_task(task);

	kfree(task->tsk.stack);
	kfree(task->name);
	kfree(task);
}


/**
 * @brief wake up a kthread
 *
 * Enqueues the task in the scheduler and transitions it from TASK_NEW
 * to a runnable state. Sends an IPI reschedule if the task has a
 * specific CPU affinity.
 *
 * @param task the task to wake up (must be in TASK_NEW state)
 *
 * @return 0 on success, or a negative error code
 */

int kthread_wake_up(struct task_struct *task)
{
	int ret = 0;

	unsigned long flags;

	ktime now;


	if (!task)
		return -EINVAL;

	if (task->state != TASK_NEW)
		return -EINVAL;

	ret = sched_enqueue(task);
	if (ret)
		return ret;

	flags = arch_local_irq_save();
	kthread_lock();
	now = ktime_get();

	sched_wake(task, now);

	task->wakeup_first = now;

	/* this may be a critical task, send reschedule */
	if (task->on_cpu != KTHREAD_CPU_AFFINITY_NONE)
		smp_send_reschedule(task->on_cpu);

	kthread_unlock();
	arch_local_irq_restore(flags);

	return 0;
}


/**
 * @brief convert the boot path to a thread
 *
 * Creates the initial kernel task for the current CPU and registers it
 * with the scheduler.
 *
 * @note this function sets the initial task for any cpu; if a task has already
 *	 been set, the attempt will be rejected
 *
 * @return pointer to the created task, or a negative error pointer on failure
 */

struct task_struct *kthread_init_main(void)
{
	int cpu;

	unsigned long flags;

	struct task_struct *task;


	cpu = smp_cpu_id();
	if (current_set[cpu])
		return ERR_PTR(-EPERM);

	task = kmalloc(sizeof(*task));
	if (!task)
		return ERR_PTR(-ENOMEM);


	task->active = &task->tsk;

	sched_set_policy_default(task);

	task->state  = TASK_NEW;
	task->name   = strdup("KERNEL");
	task->on_cpu = cpu;

	arch_promote_to_task(task->active, task);

	flags = arch_local_irq_save();
	kthread_lock();

	current_set[cpu] = &task->active->thread_info;

	sched_enqueue(task);
	sched_wake(task, ktime_get());

	smp_send_reschedule(cpu);

	kthread_unlock();
	arch_local_irq_restore(flags);

	return task;
}



/**
 * @brief create the core setup of a new thread
 */

static int kthread_setup_core(struct task_struct *task,
			      struct task_core *core,
			      int (*thread_fn)(void *data), void *data)
{
#ifdef CONFIG_PAINT_KERNEL_STACK
	core->stack = kmalloc(CONFIG_STACK_SIZE);
#else
	core->stack = kzalloc(CONFIG_STACK_SIZE);
#endif /* CONFIG_PAINT_KERNEL_STACK */
	if (!core->stack)
		return -1;

#ifdef CONFIG_PAINT_KERNEL_STACK
	/* initialise stack with pattern, makes detection of errors easier */
	memset32(core->stack, 0xdeadbeef, CONFIG_STACK_SIZE / sizeof(uint32_t));
#endif /* CONFIG_PAINT_KERNEL_STACK */

	core->stack_bottom = core->stack;
	core->stack_top    = (void *)((uint8_t *)core->stack + CONFIG_STACK_SIZE);

	arch_init_task(core, task, thread_fn, data);

	return 0;
}



/**
 * @brief create a new thread
 */

static struct task_struct *kthread_create_internal(int (*thread_fn)(void *data),
						   void *data, int cpu,
						   const char *namefmt,
						   va_list args)
{
	struct task_struct *task;


	task = kzalloc(sizeof(*task));
	if (!task)
		return ERR_PTR(-ENOMEM);

	/* NOTE: we require that malloc always returns properly aligned memory,
	 * i.e. aligned to the largest possible memory access instruction
	 * (which is typically 64 bits)
	 */

	task->active = &task->tsk;

	if (kthread_setup_core(task, task->active, thread_fn, data)) {
		kfree(task);
		return ERR_PTR(-ENOMEM);
	}


	task->name = kmalloc(TASK_NAME_LEN + 1);
	vsnprintf(task->name, TASK_NAME_LEN, namefmt, args);

	if (sched_set_policy_default(task)) {
		pr_crit("KTHREAD: task policy error\n");
		kthread_free(task);
		return NULL;
	}

	INIT_LIST_HEAD(&task->ksig_queue);
	INIT_LIST_HEAD(&task->ksig_handlers);

	task->create = ktime_get();
	task->on_cpu = cpu;
	task->state  = TASK_NEW;

	pr_info("task at %p, stack %08x - %08x name %s\n", task,
							   task->active->stack_bottom,
							   task->active->stack_top,
							   task->name);

	return task;
}


/**
 * @brief create a signal subtask
 *
 * Allocates and sets up a signal handler subtask for the given task.
 * Does nothing if the task already has a signal subtask.
 *
 * @param task the parent task to attach the signal subtask to
 * @param thread_fn the function to run in the signal subtask
 *
 * @return 0 on success, or a negative error code
 */

int kthread_sigstack_create(struct task_struct *task,
			   int (*thread_fn)(void *data))
{
	if (task->sig)
		return 0;

	task->sig = kzalloc(sizeof(*task->sig));
	if(!task->sig)
		return -ENOMEM;

	return kthread_setup_core(task, task->sig, thread_fn, task);
}


/**
 * @brief create a new kernel thread
 *
 * Allocates and initialises a new kernel thread with the specified entry
 * function, CPU affinity, and name. The thread is not started until
 * kthread_wake_up() is called.
 *
 * @param thread_fn the function to run in the thread
 * @param data a user data pointer for thread_fn, may be NULL
 *
 * @param cpu set the cpu affinity
 *
 * @param namefmt a printf format string name for the thread
 *
 * @param ... parameters to the format string
 *
 * @return pointer to the created task, or a negative error pointer on failure
 */

struct task_struct *kthread_create(int (*thread_fn)(void *data),
				   void *data, int cpu,
				   const char *namefmt,
				   ...)
{
	struct task_struct *task;
	va_list args;

	va_start(args, namefmt);
	task = kthread_create_internal(thread_fn, data, cpu, namefmt, args);
	va_end(args);

	return task;
}
EXPORT_SYMBOL(kthread_create);

