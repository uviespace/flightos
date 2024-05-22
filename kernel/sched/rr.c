/**
 * @file kernel/sched/round_robin.c
 *
 * @brief round-robin scheduler
 *
 * Selects the first non-busy task which can run on the current CPU.
 * If a task has used up its runtime, the runtime is reset and the task is
 * moved to the end of the queue.
 *
 * Task runtimes are calculated from their priority value, which acts as a
 * multiplier for a given minimum slice, which is a multiple of the
 * minimum tick device period.
 *
 */


#include <errno.h>
#include <kernel/init.h>
#include <kernel/tick.h>
#include <kernel/kthread.h>
#include <kernel/smp.h>
#include <asm/spinlock.h>
#include <asm-generic/irqflags.h>


#define MSG "SCHED_RR: "


/* radix-2 shift for min tick */
#define RR_MIN_TICK_SHIFT	4

static struct spinlock rr_spinlock;

extern struct thread_info *current_set[];	/* XXX meh... */

/**
 * @brief lock critical rr section
 */

static void rr_lock(void)
{
	spin_lock_raw(&rr_spinlock);
}


/**
 * @brief unlock critical rr section
 */

static void rr_unlock(void)
{
	spin_unlock(&rr_spinlock);
}


/**
 * @brief select the next task to run
 */

static struct task_struct *rr_pick_next(struct task_queue tq[], int cpu,
					ktime now)
{
	ktime tick;

	struct task_struct *tsk;
	struct task_struct *tmp;
	struct task_struct *next = NULL;



	if (list_empty(&tq[0].run))
		return NULL;


	/* we use twice the minimum tick period for resetting the runtime */
	tick = (ktime) tick_get_period_min_ns() << 1;

	rr_lock();

	list_for_each_entry_safe(tsk, tmp, &tq[0].run, node) {

		if (tsk->on_cpu != cpu)
			if (tsk->on_cpu != KTHREAD_CPU_AFFINITY_NONE)
				continue;


		if (tsk->state == TASK_RUN) {

			if (tsk->runtime <= tick) {
				/* reset runtime and queue up at the end */
				tsk->runtime = tsk->attr.wcet;
				list_move_tail(&tsk->node, &tq[0].run);
				next = tsk;
				continue;
			}

			next = tsk;
			break;
		}


		if (tsk->state == TASK_DEAD)
			if (tsk->on_cpu == cpu)
				list_move_tail(&tsk->node, &tq[cpu].dead);
	}

	if (next)
		next->state = TASK_BUSY;

	rr_unlock();

	return next;
}

static int rr_cleanup(void *data)
{
	int cpu;

	struct task_struct *tsk;
	struct task_struct *tmp;
	unsigned long flags;

	struct task_queue *tq;


	cpu = smp_cpu_id();

	tq = ((struct scheduler *)data)->tq;

	while (1) {

		/* always explicitly yield here; this ensures that a DEAD
		 * task has been removed and its stack is no longer in use
		 * when we next encounter it on the to-clean list
		 */

		sched_yield();

		list_for_each_entry_safe(tsk, tmp, &tq[cpu].clean, node) {
			list_del(&tsk->node);
			kthread_free(tsk);
		}

		/* the scheduler moves any task encounter to the "dead"
		 * list if the next task it encounters is so marked;
		 * we then briefly lock the dead list and move the
		 * task to the to-clean list; this allows us to keep
		 * locking time at a minimum and also ensures
		 * consistency because we are the only users
		 * of the clean list in this particular function
		 */

		flags = arch_local_irq_save();
		rr_lock();

		list_for_each_entry_safe(tsk, tmp, &tq[cpu].dead, node)
			list_move_tail(&tsk->node, &tq[cpu].clean);


		rr_unlock();
		arch_local_irq_restore(flags);
	}

	return 0;
}

/**
 * @brief wake up a task by inserting in into the run queue
 */

static int rr_wake(struct task_struct *task, ktime now)
{
	int found = 0;

	ktime tick;

	struct task_struct *elem;
	struct task_struct *tmp;

	struct task_queue *tq;

	unsigned long flags;


	if (!task)
		return -EINVAL;

	if (task->attr.policy != SCHED_RR)
		return -EINVAL;


	tq = task->sched->tq;
	if (list_empty(&tq[0].wake))
		return -EINVAL;


	list_for_each_entry_safe(elem, tmp, &tq[0].wake, node) {

		if (elem != task)
			continue;

		found = 1;
		break;
	}

	if (!found)
		return -EINVAL;


	/* let's hope tick periods between cpus never differ significantly */
	tick = (ktime) tick_get_period_min_ns() << RR_MIN_TICK_SHIFT;

	task->attr.wcet = task->attr.priority * tick;
	task->runtime   = task->attr.wcet;
	task->state     = TASK_RUN;

	flags = arch_local_irq_save();
	rr_lock();
	list_move(&task->node, &tq[0].run);
	rr_unlock();
	arch_local_irq_restore(flags);


	return 0;
}



/**
 * @brief enqueue a task
 */

static int rr_enqueue(struct task_struct *task)
{
	unsigned long flags;

	flags = arch_local_irq_save();
	rr_lock();
	list_add_tail(&task->node, &task->sched->tq[0].wake);
	rr_unlock();
	arch_local_irq_restore(flags);

	return 0;
}


/**
 * @brief get the requested timeslice of a task
 */

static ktime rr_timeslice_ns(struct task_struct *task)
{
	return task->runtime;
}


/**
 * @brief sanity-check sched_attr configuration
 *
 * @return 0 on success -EINVAL on error
 */

static int rr_check_sched_attr(struct sched_attr *attr)
{

	if (!attr)
		return -EINVAL;

	if (attr->policy != SCHED_RR) {
		pr_err(MSG "attribute policy is %d, expected SCHED_RR (%d)\n",
		            attr->policy, SCHED_RR);
		return -EINVAL;
	}

	if (!attr->priority) {
		attr->priority = 1;
		pr_warn(MSG "minimum priority is 1, adjusted\n");
	}

	return 0;
}


/**
 * @brief return the time until the the next task is ready
 *
 * @note RR tasks are always "ready" and they do not have deadlines,
 *	 so this function always returns 0
 */

ktime rr_task_ready_ns(struct task_queue tq[], int cpu, ktime now)
{
	return (ktime) 0ULL;
}


static struct scheduler sched_rr = {
	.policy           = SCHED_RR,
	.pick_next_task   = rr_pick_next,
	.wake_task        = rr_wake,
	.enqueue_task     = rr_enqueue,
	.timeslice_ns     = rr_timeslice_ns,
	.task_ready_ns    = rr_task_ready_ns,
	.check_sched_attr = rr_check_sched_attr,
	.priority   = 0,
};



static int sched_rr_init(void)
{
	int i;


	for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++) {
		INIT_LIST_HEAD(&sched_rr.tq[i].wake);
		INIT_LIST_HEAD(&sched_rr.tq[i].run);
		INIT_LIST_HEAD(&sched_rr.tq[i].dead);
		INIT_LIST_HEAD(&sched_rr.tq[i].clean);
	}


	sched_register(&sched_rr);

	return 0;
}
postcore_initcall(sched_rr_init);


static int sched_rr_cleanup_init(void)
{
	int i;

	struct task_struct *t;

	/* need one cleanup per cpu */
	for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++) {
		t = kthread_create(rr_cleanup, &sched_rr, i, "RR_CLEAN");
		BUG_ON(!t);
		BUG_ON(kthread_wake_up(t) < 0);
	}

	return 0;
}
late_initcall(sched_rr_cleanup_init);
