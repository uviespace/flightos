/**
 * @file kernel/kthread.c
 */


#include <kernel/kthread.h>
#include <kernel/export.h>
#include <kernel/kmem.h>
#include <kernel/err.h>
#include <kernel/printk.h>

#include <asm-generic/irqflags.h>
#include <asm-generic/spinlock.h>


#include <asm/switch_to.h>

#include <kernel/string.h>

#include <kernel/tick.h>



#define MSG "KTHREAD: "


static struct {
	struct list_head new;
	struct list_head run;
	struct list_head idle;
	struct list_head dead;
} _kthreads = {
	.new  = LIST_HEAD_INIT(_kthreads.new),
	.run  = LIST_HEAD_INIT(_kthreads.run),
	.idle = LIST_HEAD_INIT(_kthreads.idle),
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
		kfree(p_elem->name);
		kfree(p_elem);
	}
}


void sched_print_edf_list(void)
{
	ktime now;
	char state = 'U';
	int64_t rel_deadline;
	int64_t rel_wait;

	struct task_struct *tsk;
	struct task_struct *tmp;


	now = ktime_get();


	printk("\nt: %lld\n", ktime_to_us(now));
	printk("S\tDeadline\tWakeup\tdelta W\tdelta P\tt_rem\tName\n");
	printk("----------------------------------------------\n");

	list_for_each_entry_safe(tsk, tmp, &_kthreads.run, node) {

		if (tsk->policy != SCHED_EDF)
			continue;

		rel_deadline = ktime_delta(tsk->deadline, now);
		rel_wait     = ktime_delta(tsk->wakeup,   now);
		if (rel_wait < 0)
			rel_wait = 0; /* running */

		if (tsk->state == TASK_IDLE)
			state = 'I';
		if (tsk->state == TASK_RUN)
			state = 'R';

		printk("%c\t%lld\t%lld\t%lld\t%lld\t%lld\t%s\n",
		       state, ktime_to_us(tsk->deadline), ktime_to_us(tsk->wakeup),
		       ktime_to_us(rel_wait), ktime_to_us(rel_deadline), ktime_to_us(tsk->runtime), tsk->name);
	}

}

/**
 * Our EDF task scheduling timeline:
 *
 *
 *
 *  wakeup/
 *  activation
 *   |                      absolute
 *   |                      deadline
 *   |  start                  |
 *   |  time                   |               next
 *   |   |                     |              wakeup
 *   |   | computation|        |                |
 *   |   |    time    |        |                |
 *   |   |############|        |                |
 *   +-----+-------------------+-----------------
 *         |------ WCET -------|
 *         ^- latest start time
 *   |--- relative deadline ---|
 *   |---------------- period ------------------|
 *
 *
 *
 *
 */


/**
 * @brief check if an EDF task can still execute given its deadline
 *
 * @note effectively checks
 *	 wcet         remaining runtime in slot
 *	------   <   --------------------------
 *	period        remaining time to deadline
 *
 * @returns true if can still execute before deadline
 */

static inline bool schedule_edf_can_execute(struct task_struct *tsk, ktime now)
{
	int64_t rel_deadline;


	if (tsk->runtime <= 0)
		return false;

	rel_deadline = ktime_delta(tsk->deadline, now);
	if (rel_deadline <= 0)
		return false;

	if (tsk->wcet * rel_deadline < tsk->period * tsk->runtime)
		return true;

	return false;
}


static inline void schedule_edf_reinit_task(struct task_struct *tsk, ktime now)
{
	tsk->state = TASK_IDLE;

	tsk->wakeup = ktime_add(tsk->wakeup, tsk->period);
//	BUG_ON(ktime_after(tsk->wakeup, now)); /* deadline missed earlier? */

	tsk->deadline = ktime_add(tsk->wakeup, tsk->deadline_rel);

	tsk->runtime = tsk->wcet;
}


#define SOME_DEFAULT_TICK_PERIOD_FOR_SCHED_MODE 3000000000UL
/* stupidly sort EDFs */
static int64_t schedule_edf(ktime now)
{
//	ktime now;

	int64_t delta;

	int64_t slot = SOME_DEFAULT_TICK_PERIOD_FOR_SCHED_MODE;

	struct task_struct *tsk;
	struct task_struct *tmp;

	ktime wake;

//	now = ktime_get();

//	printk("vvvv\n");
	list_for_each_entry_safe(tsk, tmp, &_kthreads.run, node) {

		if (tsk->policy != SCHED_EDF)
			continue;

	//	printk("%s: %lld\n", tsk->name, ktime_to_us(tsk->wakeup));
		/* time to wake up yet? */
		if (ktime_after(tsk->wakeup, now)) {
			/* nope, update minimum runtime for this slot */
			delta = ktime_delta(tsk->wakeup, now);
			if (delta > 0  && (delta < slot))
				slot = delta;
			continue;
		}

		/* if it's already running, see if there is time remaining */
		if (tsk->state == TASK_RUN) {
			if (ktime_after(tsk->wakeup, now)) {
				printk("violated %s\n", tsk->name);
			}
			if (!schedule_edf_can_execute(tsk, now)) {
				schedule_edf_reinit_task(tsk, now);
				/* nope, update minimum runtime for this slot */
				delta = ktime_delta(tsk->wakeup, now);
				if (delta > 0  && (delta < slot))
					slot = delta;
				continue;
			}

			/* move to top */
			list_move(&tsk->node, &_kthreads.run);
			continue;
		}

		/* time to wake up */
		if (tsk->state == TASK_IDLE) {
			tsk->state = TASK_RUN;
			/* move to top */
			list_move(&tsk->node, &_kthreads.run);
		}

	}
//	printk("---\n");
	/* now find the closest relative deadline */

	wake = ktime_add(now, slot);
	list_for_each_entry_safe(tsk, tmp, &_kthreads.run, node) {

		if (tsk->policy != SCHED_EDF)
			break;

		/* all currently runnable task are at the top of the list */
		if (tsk->state != TASK_RUN)
			break;

		if (ktime_before(wake, tsk->deadline))
			continue;

		delta = ktime_delta(wake, tsk->deadline);

		if (delta < 0) {
			delta = ktime_delta(now, tsk->deadline);
			printk("\n [%lld] %s deadline violated by %lld us\n", ktime_to_ms(now), tsk->name, ktime_to_us(delta));
		}


		if (delta < slot) {
			if (delta)
				slot = delta;
			else
				delta = tsk->runtime;
			wake = ktime_add(now, slot); /* update next wakeup */
			/* move to top */
			list_move(&tsk->node, &_kthreads.run);
			BUG_ON(slot <= 0);
		}
	}

//	printk("in: %lld\n", ktime_to_us(ktime_delta(ktime_get(), now)));
	BUG_ON(slot < 0);

	return slot;
}


void sched_yield(void)
{
	struct task_struct *tsk;

	tsk = current_set[0]->task;
	if (tsk->policy == SCHED_EDF)
		tsk->runtime = 0;

	schedule();
}



void schedule(void)
{
	struct task_struct *next;
	struct task_struct *current;
	int64_t slot_ns;
	ktime now = ktime_get();


	if (list_empty(&_kthreads.run))
		return;



	/* XXX: dummy; need solution for sched_yield() so timer is
	 * disabled either via disable_tick or need_resched variable
	 * (in thread structure maybe?)
	 * If we don't do this here, the timer may fire while interrupts
	 * are disabled, which may land us here yet again
	 */


	arch_local_irq_disable();

	kthread_lock();
	tick_set_next_ns(1e9);



	current = current_set[0]->task;
	if (current->policy == SCHED_EDF) {
		ktime d;
		d = ktime_delta(now, current->exec_start);
//		if (d > current->runtime);
//			printk("\ntrem: %lld, %lld\n", ktime_to_us(current->runtime), ktime_to_us(d));
		if (current->runtime)
			current->runtime -= d;
	}


	/** XXX not here, add cleanup thread */
	kthread_cleanup_dead();

#if 1
	{
		static int init;
		if (!init) { /* dummy switch */
			tick_set_mode(TICK_MODE_ONESHOT);
			init = 1;
		}
	}
#endif
	slot_ns = schedule_edf(now);

	/* round robin as before */
	do {

		next = list_entry(_kthreads.run.next, struct task_struct, node);

		//printk("[%lld] %s\n", ktime_to_ms(ktime_get()), next->name);
		if (!next)
			BUG();

		if (next->state == TASK_RUN) {
			list_move_tail(&next->node, &_kthreads.run);
			break;
		}
		if (next->state == TASK_IDLE) {
			list_move_tail(&next->node, &_kthreads.run);
			continue;
		}

		if (next->state == TASK_DEAD)
			list_move_tail(&next->node, &_kthreads.dead);

	} while (!list_empty(&_kthreads.run));


	if (next->policy == SCHED_EDF) {
		if (next->runtime <= slot_ns) {
			slot_ns = next->runtime; /* XXX must track actual time because of IRQs */
			next->runtime = 0;
		}
	}

	next->exec_start = now;

	kthread_unlock();

	if (slot_ns > 0xffffffff)
		slot_ns = 0xffffffff;

	if (slot_ns < 18000) {
		printk("%u\n",slot_ns);
		slot_ns = 18000;
	}
//	printk("\n%lld ms\n", ktime_to_ms(slot_ns));
	tick_set_next_ns(slot_ns);
#if 0
#if 0
	tick_set_next_ns(30000);
#else
	{
		static int cnt = 2;
		static int sig = 1;
		struct timespec now;
		now = get_ktime();
		now.tv_nsec += 1000000000 / cnt; // 10k Hz

		if (cnt > 100)
			sig = -1;
		if (cnt < 2)
			sig = 1;
		cnt = cnt + sig;
		BUG_ON(tick_set_next_ktime(now) < 0);
	}
#endif
#endif

	prepare_arch_switch(1);
	switch_to(next);

	arch_local_irq_enable();
}


static void kthread_set_sched_policy(struct task_struct *task,
				     enum sched_policy policy)
{
	arch_local_irq_disable();
	kthread_lock();
	task->policy = policy;
	kthread_unlock();
	arch_local_irq_enable();
}


void kthread_set_sched_edf(struct task_struct *task, unsigned long period_us,
			  unsigned long wcet_us, unsigned long deadline_rel_us)
{
	/* XXX schedulability tests */

	if (wcet_us >= period_us) {
		printk("Cannot schedule EDF task with WCET %u >= PERIOD %u !\n", wcet_us, period_us);
		return;
	}

	if (wcet_us >= deadline_rel_us) {
		printk("Cannot schedule EDF task with WCET %u >= DEADLINE %u !\n", wcet_us, deadline_rel_us);
		return;
	}

	if (deadline_rel_us >= period_us) {
		printk("Cannot schedule EDF task with DEADLINE %u >= PERIOD %u !\n", deadline_rel_us, period_us);
		return;
	}


	arch_local_irq_disable();
	task->period   = us_to_ktime(period_us);
	task->wcet     = us_to_ktime(wcet_us);
	task->runtime  = task->wcet;
	task->deadline_rel = us_to_ktime(deadline_rel_us);




	{
		double u = 0.0;	/* utilisation */

		struct task_struct *tsk;
		struct task_struct *tmp;


		u += (double) (int32_t) task->wcet / (double) (int32_t) task->period;


		list_for_each_entry_safe(tsk, tmp, &_kthreads.run, node) {

			if (tsk->policy != SCHED_EDF)
				continue;

			u += (double) (int32_t) tsk->wcet / (double) (int32_t) tsk->period;
		}
		if (u > 1.0)
			printk("I am not schedule-able: %g\n", u);
	}



	kthread_set_sched_policy(task, SCHED_EDF);

	arch_local_irq_enable();





}



/**
 * t1:        | ##d
 *
 * t2:   |    #####d
 *
 * t3:      |       ############d
 *  --------------------------------------------------
 *
 * |...wakeup
 * #...wcet
 * d...deadline
 */

void kthread_wake_up(struct task_struct *task)
{
//	printk("wake thread %p\n", task->stack_top);
	arch_local_irq_disable();
	kthread_lock();

	/* for now, simply take the current time and add the task wakeup
	 * period to configure the first wakeup, then set the deadline
	 * accordingly.
	 * note: once we have more proper scheduling, we will want to
	 * consider the following: if a EDF task is in paused state (e.g.
	 * with a semaphore locked, do the same when the semaphore is unlocked,
	 * but set the deadline to now + wcet
	 */

	BUG_ON(task->state != TASK_NEW);

	task->state = TASK_IDLE;

	if (task->policy == SCHED_EDF) {

		struct task_struct *tsk;
		struct task_struct *tmp;

		/* initially set current time as wakeup */
		task->wakeup = ktime_add(ktime_get(), task->period);
		task->deadline = ktime_add(task->wakeup, task->deadline_rel);

	//	printk("%s initial wake: %llu, deadline: %llu\n", task->name, ktime_to_us(task->wakeup), ktime_to_us(task->deadline));

		list_for_each_entry_safe(tsk, tmp, &_kthreads.run, node) {

			if (tsk->policy != SCHED_EDF)
				continue;

			if (ktime_before(tsk->deadline, task->deadline))
				continue;

			/* move the deadline forward */
			task->deadline = ktime_add(tsk->deadline, task->deadline_rel);
		//	printk("%s deadline now: %llu\n", task->name, ktime_to_us(task->deadline));
		}

		/* update first wakeup time */
//		printk("%s deadline fin: %llu\n", task->name, ktime_to_us(task->deadline));
		task->wakeup = ktime_sub(task->deadline, task->deadline_rel);
//		printk("%s wakeup now: %llu\n", task->name, ktime_to_us(task->wakeup));
	}

	list_move_tail(&task->node, &_kthreads.run);

	kthread_unlock();
	arch_local_irq_enable();

	schedule();
}



struct task_struct *kthread_init_main(void)
{
	struct task_struct *task;

	task = kmalloc(sizeof(*task));

	if (!task)
		return ERR_PTR(-ENOMEM);

	/* XXX accessors */
	task->policy = SCHED_RR; /* default */

	arch_promote_to_task(task);

	task->name = "KERNEL";
	task->policy = SCHED_RR;

	arch_local_irq_disable();
	kthread_lock();

	kernel = task;
	/*** XXX dummy **/
	current_set[0] = &kernel->thread_info;


	task->state = TASK_RUN;
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

	task->stack = kmalloc(8192 + STACK_ALIGN); /* XXX */

	BUG_ON((int) task->stack > (0x40800000 - 4096 + 1));

	if (!task->stack) {
		kfree(task);
		return ERR_PTR(-ENOMEM);
	}


	task->stack_bottom = task->stack; /* XXX */
	task->stack_top = ALIGN_PTR(task->stack, STACK_ALIGN) +8192/4; /* XXX */
	BUG_ON(task->stack_top > (task->stack + (8192/4 + STACK_ALIGN/4)));

#if 0
	/* XXX: need wmemset() */
	memset(task->stack, 0xab, 8192 + STACK_ALIGN);
#else
	{
		int i;
		for (i = 0; i < (8192 + STACK_ALIGN) / 4; i++)
			((int *) task->stack)[i] = 0xdeadbeef;

	}
#endif

	/* dummy */
	task->name = kmalloc(32);
	BUG_ON(!task->name);
	vsnprintf(task->name, 32, namefmt, args);

	/* XXX accessors */
	task->policy = SCHED_RR; /* default */

	arch_init_task(task, thread_fn, data);

	task->state = TASK_NEW;

	arch_local_irq_disable();
	kthread_lock();

	list_add_tail(&task->node, &_kthreads.new);

	kthread_unlock();
	arch_local_irq_enable();


	//printk("%s is next at %p stack %p\n", namefmt, &task->thread_info, task->stack);
	//printk("%s\n", namefmt);


	return task;
}





/**
 * try_to_wake_up - wake up a thread
 * @p: the thread to be awakened
 * @state: the mask of task states that can be woken
 * @wake_flags: wake modifier flags (WF_*)
 *
 * If (@state & @p->state) @p->state = TASK_RUN.
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



