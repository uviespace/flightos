/**
 * @file kernel/sched/core.c
 *
 * @brief the core scheduling code
 */



#include <kernel/err.h>
#include <kernel/kthread.h>
#include <kernel/sched.h>
#include <kernel/init.h>
#include <kernel/tick.h>
#include <asm-generic/irqflags.h>
#include <asm-generic/spinlock.h>
#include <asm/switch_to.h>
#include <string.h>



#define MSG "SCHEDULER: "

static LIST_HEAD(kernel_schedulers);


/* XXX: per-cpu */
extern struct thread_info *current_set[1];



void schedule(void)
{
	struct scheduler *sched;

	struct task_struct *next = NULL;

	struct task_struct *current;
	int64_t slot_ns;
	int64_t wake_ns = 0;




	arch_local_irq_disable();
	/* kthread_lock(); */


	/* XXX: for now, try to wake up any threads not running
	 * this is a waste of cycles and instruction space; should be
	 * done in the scheduler's code (somewhere) */
	list_for_each_entry(sched, &kernel_schedulers, node) {
		sched->wake_next_task(&sched->tq);
	}

	current = current_set[0]->task;

	/* XXX need sorted list: highest->lowest scheduler priority, e.g.:
	 * EDF -> RMS -> FIFO -> RR
	 * TODO: scheduler priority value
	 */
	list_for_each_entry(sched, &kernel_schedulers, node) {

		next = sched->pick_next_task(&sched->tq);

		/* check if we need to limit the next tasks timeslice;
		 * since our scheduler list is sorted by scheduler priority,
		 * only update the value if wake_next is not set;
		 * we assume that the timeslice is reasonable; if not fix it in
		 * the corresponding scheduler
		 */
		if (!wake_ns)
			wake_ns = sched->task_ready_ns(&sched->tq);

		if (next)
			break;
	}


	if (!next) {
		/* nothing to do, check again later */
		if (wake_ns)
			tick_set_next_ns(wake_ns);
		else
			tick_set_next_ns(1e9);	/* XXX pause for a second */

		goto exit;
	}

	/* see if we can use a full slice or if we have to wake earlier */
	if (wake_ns)
		slot_ns = wake_ns;
	else
		slot_ns = next->sched->timeslice_ns(next);

	/* statistics */
	next->exec_start = ktime_get();

	/* kthread_unlock(); */

	tick_set_next_ns(slot_ns);

	prepare_arch_switch(1);
	switch_to(next);

exit:
	arch_local_irq_enable();
}




/**
 * @brief enqueue a task
 */

int sched_enqueue(struct task_struct *task)
{
	if (!task->sched) {
		pr_err(MSG "no scheduler configured for task %s\n", task->name);
		return -EINVAL;
	}

	/** XXX retval **/
	if (task->sched->check_sched_attr(&task->attr))
		return -EINVAL;

	task->sched->enqueue_task(&task->sched->tq, task);

	return 0;
}


/**
 * @brief set a scheduling attribute for a task
 *
 * @returns 0 on success, < 0 on error
 *
 * XXX: should implement list of all threads, so we can use pid_t pid
 *
 * XXX: no error checking of attr params
 */

int sched_set_attr(struct task_struct *task, struct sched_attr *attr)
{
	struct scheduler *sched;


	if (!task)
		goto error;

	if (!attr)
		goto error;


	list_for_each_entry(sched, &kernel_schedulers, node) {

		if (sched->policy == attr->policy) {

			memcpy(&task->attr, attr, sizeof(struct sched_attr));

			if (sched->check_sched_attr(attr))
				goto error;

			task->sched  = sched;

			/* XXX other stuff */

			return 0;
		}
	}

	pr_crit(MSG "specified policy %d not available\n", attr->policy);

error:
	task->sched = NULL;
	return -EINVAL;
}


/**
 * @brief get a scheduling attribute for a task
 * XXX: should implement list of all threads, so we can use pid_t pid
 */

int sched_get_attr(struct task_struct *task, struct sched_attr *attr)
{

	if (!task)
		return -EINVAL;

	if (!attr)
		return -EINVAL;


	memcpy(attr, &task->attr, sizeof(struct sched_attr));


	return 0;
}


/**
 * @brief set a task to the default scheduling policy
 */

int sched_set_policy_default(struct task_struct *task)
{
	struct sched_attr attr = {.policy = SCHED_RR,
				  .priority = 1};

	return sched_set_attr(task, &attr);
}


/**
 * @brief register a new scheduler
 */

int sched_register(struct scheduler *sched)
{
	/* XXX locks */


	/* XXX stupid */
	if (!sched->sched_priority)
		list_add_tail(&sched->node, &kernel_schedulers);
	else
		list_add(&sched->node, &kernel_schedulers);

	return 0;
}


/**
 * @brief scheduler initcall
 *
 * @note sets tick mode to oneshot
 */

static int sched_init(void)
{
	tick_set_mode(TICK_MODE_ONESHOT);
	tick_set_next_ns(1e9);	/* XXX default to 1s ticks initially */

	return 0;
}
late_initcall(sched_init);
