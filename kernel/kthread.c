/**
 * @file kernel/kthread.c
 */


#include <kernel/kthread.h>
#include <kernel/export.h>
#include <kernel/kmem.h>
#include <kernel/err.h>
#include <kernel/printk.h>

#include <asm/spinlock.h>
#include <asm/switch_to.h>
#include <asm/irqflags.h>

#include <string.h>


#define MSG "KTHREAD: "


static struct {
	struct list_head new;
	struct list_head run;
	struct list_head dead;
} _kthreads = {
	.new  = LIST_HEAD_INIT(_kthreads.new),
	.run  = LIST_HEAD_INIT(_kthreads.run),
	.dead = LIST_HEAD_INIT(_kthreads.dead)
};

static struct spinlock kthread_spinlock;


/** XXX dummy **/
struct task_struct *kernel;


struct thread_info *current_set[1];


/**
 * @brief lock critical kthread section
 */

static inline void kthread_lock(void)
{
	spin_lock(&kthread_spinlock);
}


/**
 * @brief unlock critical kthread section
 */

static inline void kthread_unlock(void)
{
	spin_unlock(&kthread_spinlock);
}


/* this should be a thread with a semaphore
 * that is unlocked by schedule() if dead tasks
 * were added
 * (need to irq_disable/kthread_lock)
 */

void kthread_cleanup_dead(void)
{
	struct task_struct *p_elem;
	struct task_struct *p_tmp;

	list_for_each_entry_safe(p_elem, p_tmp, &_kthreads.dead, node) {
		list_del(&p_elem->node);
		kfree(p_elem->stack);
		kfree(p_elem);
	}
}




void schedule(void)
{
	struct task_struct *next;


	if (list_empty(&_kthreads.run))
	    return;




	arch_local_irq_disable();

	kthread_lock();
	kthread_cleanup_dead();



	/* round robin */
	do {
		next = list_entry(_kthreads.run.next, struct task_struct, node);
		if (!next)
			BUG();

		if (next->state == TASK_RUNNING) {
			list_move_tail(&next->node, &_kthreads.run);
			break;
		}

		list_move_tail(&next->node, &_kthreads.dead);

	} while (!list_empty(&_kthreads.run));

	kthread_unlock();




	prepare_arch_switch(1);
	switch_to(next);

	arch_local_irq_enable();
}



void kthread_wake_up(struct task_struct *task)
{
	printk("wake thread %p\n", task->stack_top);
	arch_local_irq_disable();
	kthread_lock();
	task->state = TASK_RUNNING;
	list_move_tail(&task->node, &_kthreads.run);
	kthread_unlock();
	arch_local_irq_enable();
}



struct task_struct *kthread_init_main(void)
{
	struct task_struct *task;

	task = kmalloc(sizeof(*task));

	if (!task)
		return ERR_PTR(-ENOMEM);


	arch_promote_to_task(task);

	arch_local_irq_disable();
	kthread_lock();

	kernel = task;
	/*** XXX dummy **/
	current_set[0] = &kernel->thread_info;

	task->state = TASK_RUNNING;
	list_add_tail(&task->node, &_kthreads.run);



	kthread_unlock();
	arch_local_irq_enable();

	return task;
}




static struct task_struct *kthread_create_internal(int (*thread_fn)(void *data),
						   void *data, int cpu,
						   const char *namefmt,
						   va_list args)
{
	struct task_struct *task;

	task = kmalloc(sizeof(*task));


	if (!task)
		return ERR_PTR(-ENOMEM);


	/* XXX: need stack size detection and realloc/migration code */

	task->stack = kmalloc(8192); /* XXX */

	if (!task->stack) {
		kfree(task);
		return ERR_PTR(-ENOMEM);
	}


	task->stack_bottom = task->stack; /* XXX */
	task->stack_top = task->stack + 8192/4; /* XXX need align */

	memset(task->stack, 0xdeadbeef, 8192);

	arch_init_task(task, thread_fn, data);

	task->state = TASK_NEW;

	arch_local_irq_disable();
	kthread_lock();

	list_add_tail(&task->node, &_kthreads.new);

	kthread_unlock();
	arch_local_irq_enable();


	//printk("%s is next at %p stack %p\n", namefmt, &task->thread_info, task->stack);
	printk("%s\n", namefmt);


	return task;
}





/**
 * try_to_wake_up - wake up a thread
 * @p: the thread to be awakened
 * @state: the mask of task states that can be woken
 * @wake_flags: wake modifier flags (WF_*)
 *
 * If (@state & @p->state) @p->state = TASK_RUNNING.
 *
 * If the task was not queued/runnable, also place it back on a runqueue.
 *
 * Atomic against schedule() which would dequeue a task, also see
 * set_current_state().
 *
 * Return: %true if @p->state changes (an actual wakeup was done),
 *	   %false otherwise.
 */
static int
wake_up_thread_internal(struct task_struct *p, unsigned int state, int wake_flags)
{
	//unsigned long flags;
	//int cpu = 0;
	int success = 0;
#if 0
	/*
	 * If we are going to wake up a thread waiting for CONDITION we
	 * need to ensure that CONDITION=1 done by the caller can not be
	 * reordered with p->state check below. This pairs with mb() in
	 * set_current_state() the waiting thread does.
	 */
	raw_spin_lock_irqsave(&p->pi_lock, flags);
	smp_mb__after_spinlock();
	if (!(p->state & state))
		goto out;

	trace_sched_waking(p);

	/* We're going to change ->state: */
	success = 1;
	cpu = task_cpu(p);

	/*
	 * Ensure we load p->on_rq _after_ p->state, otherwise it would
	 * be possible to, falsely, observe p->on_rq == 0 and get stuck
	 * in smp_cond_load_acquire() below.
	 *
	 * sched_ttwu_pending()                 try_to_wake_up()
	 *   [S] p->on_rq = 1;                  [L] P->state
	 *       UNLOCK rq->lock  -----.
	 *                              \
	 *				 +---   RMB
	 * schedule()                   /
	 *       LOCK rq->lock    -----'
	 *       UNLOCK rq->lock
	 *
	 * [task p]
	 *   [S] p->state = UNINTERRUPTIBLE     [L] p->on_rq
	 *
	 * Pairs with the UNLOCK+LOCK on rq->lock from the
	 * last wakeup of our task and the schedule that got our task
	 * current.
	 */
	smp_rmb();
	if (p->on_rq && ttwu_remote(p, wake_flags))
		goto stat;

#ifdef CONFIG_SMP
	/*
	 * Ensure we load p->on_cpu _after_ p->on_rq, otherwise it would be
	 * possible to, falsely, observe p->on_cpu == 0.
	 *
	 * One must be running (->on_cpu == 1) in order to remove oneself
	 * from the runqueue.
	 *
	 *  [S] ->on_cpu = 1;	[L] ->on_rq
	 *      UNLOCK rq->lock
	 *			RMB
	 *      LOCK   rq->lock
	 *  [S] ->on_rq = 0;    [L] ->on_cpu
	 *
	 * Pairs with the full barrier implied in the UNLOCK+LOCK on rq->lock
	 * from the consecutive calls to schedule(); the first switching to our
	 * task, the second putting it to sleep.
	 */
	smp_rmb();

	/*
	 * If the owning (remote) CPU is still in the middle of schedule() with
	 * this task as prev, wait until its done referencing the task.
	 *
	 * Pairs with the smp_store_release() in finish_task().
	 *
	 * This ensures that tasks getting woken will be fully ordered against
	 * their previous state and preserve Program Order.
	 */
	smp_cond_load_acquire(&p->on_cpu, !VAL);

	p->sched_contributes_to_load = !!task_contributes_to_load(p);
	p->state = TASK_WAKING;

	if (p->in_iowait) {
		delayacct_blkio_end(p);
		atomic_dec(&task_rq(p)->nr_iowait);
	}

	cpu = select_task_rq(p, p->wake_cpu, SD_BALANCE_WAKE, wake_flags);
	if (task_cpu(p) != cpu) {
		wake_flags |= WF_MIGRATED;
		set_task_cpu(p, cpu);
	}

#else /* CONFIG_SMP */

	if (p->in_iowait) {
		delayacct_blkio_end(p);
		atomic_dec(&task_rq(p)->nr_iowait);
	}

#endif /* CONFIG_SMP */

	ttwu_queue(p, cpu, wake_flags);
stat:
	ttwu_stat(p, cpu, wake_flags);
out:
	raw_spin_unlock_irqrestore(&p->pi_lock, flags);
#endif
	return success;
}

/**
 * wake_up_process - Wake up a specific process
 * @p: The process to be woken up.
 *
 * Attempt to wake up the nominated process and move it to the set of runnable
 * processes.
 *
 * Return: 1 if the process was woken up, 0 if it was already running.
 *
 * It may be assumed that this function implies a write memory barrier before
 * changing the task state if and only if any tasks are woken up.
 */
/* Used in tsk->state: */

int wake_up_thread(struct task_struct *p)
{
	return wake_up_thread_internal(p, 0xdead, 0);
}
EXPORT_SYMBOL(wake_up_thread);


/**
 *
 * @brief create a new kernel thread
 *
 * @param thread_fn the function to run in the thread
 * @param data a user data pointer for thread_fn, may be NULL
 *
 * @param cpu set the cpu affinity
 *
 * @param name_fmt a printf format string name for the thread
 *
 * @param ... parameters to the format string
 *
 * Create a named kernel thread. The thread will be initially stopped.
 * Use wake_up_thread to activate it.
 *
 * If cpu is set to KTHREAD_CPU_AFFINITY_NONE, the thread will be affine to all
 * CPUs. IF the selected CPU index excceds the number of available CPUS, it
 * will default to KTHREAD_CPU_AFFINITY_NONE, otherwise the thread will be
 * bound to that CPU
 *
 * The new thread has SCHED_NORMAL policy.
 *
 * If thread is going to be bound on a particular cpu, give its node
 * in @node, to get NUMA affinity for kthread stack, or else give NUMA_NO_NODE.
 * When woken, the thread will run @threadfn() with @data as its
 * argument. @threadfn() can either call do_exit() directly if it is a
 * standalone thread for which no one will call kthread_stop(), or
 * return when 'kthread_should_stop()' is true (which means
 * kthread_stop() has been called).  The return value should be zero
 * or a negative error number; it will be passed to kthread_stop().
 *
 * Returns a task_struct or ERR_PTR(-ENOMEM) or ERR_PTR(-EINTR).
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
