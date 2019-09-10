/**
 * @file kernel/sched/round_robin.c
 *
 * @brief round-robin scheduler
 */


#include <errno.h>
#include <kernel/init.h>
#include <kernel/tick.h>
#include <kernel/kthread.h>

#define MSG "SCHED_RR: "


static struct task_struct *rr_pick_next(struct task_queue *tq)
{
	struct task_struct *next = NULL;


	while (!list_empty(&tq->run)) {

		next = list_entry(tq->run.next, struct task_struct, node);

		BUG_ON(!next);

		if (next->state == TASK_RUN) {
			/* XXX: must pick head first, then move tail on put()
			 * following a scheduling event. for now, just force
			 * round robin
			 */
			list_move_tail(&next->node, &tq->run);
			break;
		}

		if (next->state == TASK_IDLE)
			list_move_tail(&next->node, &tq->run);

		if (next->state == TASK_DEAD)
			list_move_tail(&next->node, &tq->dead);

	}


	return next;
}


/* this sucks, wrong place. keep for now */
static void rr_wake_next(struct task_queue *tq)
{

	struct task_struct *task;

	if (list_empty(&tq->wake))
		return;


	task = list_entry(tq->wake.next, struct task_struct, node);

	BUG_ON(task->attr.policy != SCHED_RR);
	/** XXX NO LOCKS */
	task->state = TASK_RUN;
	list_move(&task->node, &tq->run);
}


static void rr_enqueue(struct task_queue *tq, struct task_struct *task)
{

	/** XXX **/
	if (task->state == TASK_RUN)
		list_add_tail(&task->node, &tq->run);
	else
		list_add_tail(&task->node, &tq->wake);
}

/**
 * @brief get the requested timeslice of a task
 *
 * @note RR timeslices are determined from their configured priority
 *	 XXX: must not be 0
 *
 * @note for now, just take the minimum tick period to fit as many RR slices
 *	 as possible. This will jack up the IRQ rate, but RR tasks only run when
 *	 the system is not otherwise busy;
 *	 still, a larger (configurable) extra factor may be desirable
 */

static ktime rr_timeslice_ns(struct task_struct *task)
{
	return (ktime) (task->attr.priority * tick_get_period_min_ns() * 1000);
}



/**
 * @brief sanity-check sched_attr configuration
 *
 * @return 0 on success -EINVAL on error
 */

static int rr_check_sched_attr(struct sched_attr *attr)
{
	if (attr->policy != SCHED_RR) {
		pr_err(MSG "attribute policy is %d, expected SCHED_RR (%d)\n",
			attr->policy, SCHED_RR);
		return -EINVAL;
	}

	if (!attr->priority) {
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

ktime rr_task_ready_ns(struct task_queue *tq)
{
	return (ktime) 0ULL;
}




static struct scheduler sched_rr = {
	.policy           = SCHED_RR,
	.pick_next_task   = rr_pick_next,
	.wake_next_task   = rr_wake_next,
	.enqueue_task     = rr_enqueue,
	.timeslice_ns     = rr_timeslice_ns,
	.task_ready_ns    = rr_task_ready_ns,
	.check_sched_attr = rr_check_sched_attr,
	.sched_priority   = 0,
};



static int sched_rr_init(void)
{
	/* XXX */
	INIT_LIST_HEAD(&sched_rr.tq.new);
	INIT_LIST_HEAD(&sched_rr.tq.run);
	INIT_LIST_HEAD(&sched_rr.tq.wake);
	INIT_LIST_HEAD(&sched_rr.tq.dead);

	sched_register(&sched_rr);

	return 0;
}
postcore_initcall(sched_rr_init);
