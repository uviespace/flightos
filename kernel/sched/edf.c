/**
 * @file kernel/sched/edf.c
 */


#include <kernel/kthread.h>
#include <kernel/export.h>
#include <kernel/kmem.h>
#include <kernel/err.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/tick.h>

#include <generated/autoconf.h> /* XXX need common CPU include */
#include <asm/processor.h>

void sched_print_edf_list_internal(struct task_queue *tq, ktime now)
{
//	ktime now;
	char state = 'U';
	int64_t rel_deadline;
	int64_t rel_wait;

	struct task_struct *tsk;
	struct task_struct *tmp;

	int cpu = leon3_cpuid();


	ktime prev = 0;
	ktime prevd = 0;

	printk("\nktime: %lld\n", ktime_to_us(now));
	printk("S\tDeadline\tWakeup\t\tdelta W\tdelta P\tt_rem\ttotal\tslices\tName\twcet\tavg\n");
	printk("---------------------------------------------------------------------------------------------------------------------------------\n");
	list_for_each_entry_safe(tsk, tmp, &tq->run[cpu], node) {

		if (tsk->attr.policy == SCHED_RR)
			continue;

		rel_deadline = ktime_delta(tsk->deadline, now);
		rel_wait     = ktime_delta(tsk->wakeup,   now);
		if (rel_wait < 0)
			rel_wait = 0; /* running */

		if (tsk->state == TASK_IDLE)
			state = 'I';
		if (tsk->state == TASK_RUN)
			state = 'R';

		if (tsk->slices == 0)
			tsk->slices = 1;

		printk("%c\t%lld\t%lld\t\t%lld\t%lld\t%lld\t%lld\t%d\t%s\t|\t%lld\t%lld\n",
		       state, ktime_to_ms(tsk->deadline), ktime_to_ms(tsk->wakeup),
		       ktime_to_ms(rel_wait), ktime_to_ms(rel_deadline), ktime_to_ms(tsk->runtime), ktime_to_ms(tsk->total),
		       tsk->slices, tsk->name,
		       ktime_to_ms(tsk->attr.wcet),
		       ktime_to_ms(tsk->total/tsk->slices));

		prev = tsk->wakeup;
		prevd = tsk->deadline;
	}

	printk("\n\n");
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
	int64_t to_deadline;




	if (tsk->runtime <= 0)
		return false;

	if (ktime_before(tsk->deadline, now))  {
		sched_print_edf_list_internal(&tsk->sched->tq, now);
		printk("%s violated, %lld %lld, dead %lld wake %lld now %lld start %lld\n", tsk->name,
		       tsk->runtime, ktime_us_delta(tsk->deadline, now),
		       tsk->deadline, tsk->wakeup, now, tsk->exec_start);
	//	sched_print_edf_list_internal(now);
		BUG();
		return false;
	}

	to_deadline = ktime_delta(tsk->deadline, now);

	if (to_deadline <= 0)
		return false;



	if (tsk->attr.wcet * to_deadline < tsk->attr.period * tsk->runtime)
		return true;


	return false;
}


static inline void schedule_edf_reinit_task(struct task_struct *tsk, ktime now)
{
	tsk->state = TASK_IDLE;

	tsk->wakeup = ktime_add(tsk->wakeup, tsk->attr.period);
#if 1
	if (ktime_after(now, tsk->wakeup)) {
		printk("%s delta %lld\n",tsk->name, ktime_us_delta(tsk->wakeup, now));
		printk("%s violated, %lld %lld, dead %lld wake %lld now %lld start %lld\n", tsk->name,
		       tsk->runtime, ktime_us_delta(tsk->deadline, now),
		       tsk->deadline, tsk->wakeup, now, tsk->exec_start);
	}



	BUG_ON(ktime_after(now, tsk->wakeup)); /* deadline missed earlier? */
#endif
	tsk->deadline = ktime_add(tsk->wakeup, tsk->attr.deadline_rel);

	tsk->runtime = tsk->attr.wcet;

	tsk->slices++;
}


#include <kernel/init.h>

#define MSG "SCHED_EDF: "


/**
 * @brief EDF schedulability test
 *
 * @returns 0 if schedulable, <0 otherwise
 *
 *
 * * 1) determine task with longest period
 *
 *	T1: (P=50, D=20, R=10)
 *
 * 2) calculate unused head and tail (before and after deadline)
 *
 *	UH = D1 - R1				(= 20) (Hyperperiod)
 *	UT = P1 - D1				(= 60) 
 *
 * 3) loop over other tasks (Period < Deadline of Task 1)
 *
 *	calculate slots usage before deadline of Task 1:
 *
 *	H * Ri * D1 / Pi		(T2: 10, T5: 10)
 *
 *	update head slots UH = UH - 20 = 0 -> all used
 *
 *
 *	calculate slot usage after deadline of Task2:
 *
 *	H * Ri * F1 / Pi		(T2: 15, T5: 15)
 *
 *	update tail slots: UT = UT - 30 = 30
 *
 *	-> need hyperperiod factor H = 2
 *
 *
 * ####
 *
 * Note: EDF in SMP configurations is not an optimal algorithm, and deadlines
 *	 cannot be guaranteed even for utilisation values just above 1.0
 *	 (Dhall's effect). In order to mitigate this for EDF tasks with no
 *	 CPU affinity set (KTHREAD_CPU_AFFINITY_NONE), we search the per-cpu
 *	 queues until we find one which is below the utilisation limit and
 *	 force the affinity of the task to that particular CPU
 *
 *
 *	 XXX function needs adaptation
 */

static int edf_schedulable(struct task_queue *tq, const struct task_struct *task)
{
	struct task_struct *tsk = NULL;
	struct task_struct *tmp;

	int cpu;

	double u = 0.0;	/* utilisation */



	/* add all in new */
	if (!list_empty(&tq->new)) {
		list_for_each_entry_safe(tsk, tmp, &tq->new, node) {

			u += (double) (int32_t) tsk->attr.wcet / (double) (int32_t) tsk->attr.period;

		}
	}



	/* add all in wakeup */
	if (!list_empty(&tq->wake)) {
		list_for_each_entry_safe(tsk, tmp, &tq->wake, node) {


			u += (double) (int32_t) tsk->attr.wcet / (double) (int32_t) tsk->attr.period;
		}

	}

	/* add all running */

	for (cpu = 0; cpu < 2; cpu++) {
		if (!list_empty(&tq->run[cpu])) {
			list_for_each_entry_safe(tsk, tmp, &tq->run[cpu], node)
				u += (double) (int32_t) tsk->attr.wcet / (double) (int32_t) tsk->attr.period;
		}
	}




	if (u >= 1.9) {
		printk("I am NOT schedul-ableh: %f ", u);
		BUG();
		return -EINVAL;
		printk("changed task mode to RR\n", u);
	} else {
		printk("Utilisation: %g\n", u);
		return 0;
	}


	/* TODO check against projected interrupt rate, we really need a limit
	 * there */

	return 0;
}

#include <asm/processor.h>
static struct task_struct *edf_pick_next(struct task_queue *tq, ktime now)
{
	int64_t delta;

	int cpu = leon3_cpuid();

	struct task_struct *tsk;
	struct task_struct *tmp;
	struct task_struct *first;

	static int cnt;

	if (list_empty(&tq->run[cpu]))
		return NULL;

	list_for_each_entry_safe(tsk, tmp, &tq->run[cpu], node) {

		/* time to wake up yet? */
		delta = ktime_delta(tsk->wakeup, now);

		if (delta > 0)
			continue;

		/* XXX ok: what we need here: are multiple queues: one
		 * where tasks are stored which are currently able to
		 * run, here we need one per affinity and one generic one
		 *
		 * one where tasks are stored which are currently idle.
		 * tasks move to the idle queue when they cannot execute anymore
		 * and are moved from idle to run when their wakeup time has
		 * passed
		 */

		/* if it's already running, see if there is time remaining */
		if (tsk->state == TASK_RUN) {
#if 0
			if (cnt++ > 10) {
				sched_print_edf_list_internal(&tsk->sched->tq, now);
				BUG();
			}
#endif
			if (!schedule_edf_can_execute(tsk, now)) {
				schedule_edf_reinit_task(tsk, now);

				/* always queue it up at the tail */
				list_move_tail(&tsk->node, &tq->run[cpu]);
			}


			/* if our deadline is earlier than the deadline of the
			 * head of the list, we become the new head
			 */

			first = list_first_entry(&tq->run[cpu], struct task_struct, node);

			if (ktime_before (tsk->wakeup, now)) {
				if (ktime_before (tsk->deadline - tsk->runtime, first->deadline)) {
					tsk->state = TASK_RUN;
					list_move(&tsk->node, &tq->run[cpu]);
				}
			}

			continue;
		}

		/* time to wake up */
		if (tsk->state == TASK_IDLE) {
			tsk->state = TASK_RUN;

			BUG_ON(ktime_delta(tsk->wakeup, now) > 0);

			/* if our deadline is earlier than the deadline at the
			 * head of the list, move us to top */

			first = list_first_entry(&tq->run[cpu], struct task_struct, node);

			list_move(&tsk->node, &tq->run[cpu]);

			if (ktime_before (tsk->deadline - tsk->runtime, first->deadline))
				list_move(&tsk->node, &tq->run[cpu]);

			continue;
		}

	}

	/* XXX argh, need cpu affinity run lists */
	list_for_each_entry_safe(tsk, tmp, &tq->run[cpu], node) {

		if (tsk->state == TASK_RUN) {

			tsk->state = TASK_BUSY;
			return tsk;
		}
	}

#if 0
	first = list_first_entry(&tq->run, struct task_struct, node);
	delta = ktime_delta(first->wakeup, now);



       if (first->state == TASK_RUN) {
	     //  printk("c %d\n", leon3_cpuid());
		first->state = TASK_BUSY;
	       return first;
       }
#endif

	return NULL;
}


static void edf_wake_next(struct task_queue *tq, ktime now)
{
	int cpu = leon3_cpuid();

	ktime last;

	struct task_struct *tmp;
	struct task_struct *task;
	struct task_struct *first = NULL;
	struct task_struct *t;
	struct task_struct *prev = NULL;

	ktime wake;



	if (list_empty(&tq->wake))
		return;

	last = now;
#if 0 /* OK */
	list_for_each_entry_safe(task, tmp, &tq->run, node) {
		if (last < task->deadline)
			last = task->deadline;
	}
#endif

#if 1 /* better */
	task = list_entry(tq->wake.next, struct task_struct, node);


	BUG_ON(task->on_cpu == KTHREAD_CPU_AFFINITY_NONE); 

	cpu = task->on_cpu;

	list_for_each_entry_safe(t, tmp, &tq->run[cpu], node) {

		first = list_first_entry(&tq->run[cpu], struct task_struct, node);
		if (ktime_before (t->wakeup, now)) {
			if (ktime_before (t->deadline - t->runtime, first->deadline)) {
				list_move(&t->node, &tq->run[cpu]);
			}
		}
	}

	list_for_each_entry_safe(t, tmp, &tq->run[cpu], node) {

		/* if the relative deadline of task-to-wake can fit in between the unused
		 * timeslice of this running task, insert after the next wakeup
		 */
		if (task->attr.deadline_rel < ktime_sub(t->deadline, t->wakeup)) {
		    //last = ktime_add(t->deadline, t->attr.period);
		    last = t->wakeup;


		    break;
		}

		if (task->attr.wcet < ktime_sub(t->deadline, t->wakeup)) {
		    //last = ktime_add(t->deadline, t->attr.period);
		    last = t->deadline;

		    break;
		}
	}
#endif

	task->state = TASK_IDLE;

	/* initially furthest deadline as wakeup */
	task->wakeup     = ktime_add(last, task->attr.period);
	task->deadline   = ktime_add(task->wakeup, task->attr.deadline_rel);

	/* XXX unneeded, remove */
	task->first_wake = task->wakeup;
	task->first_dead = task->deadline;


	list_move_tail(&task->node, &tq->run[cpu]);
}




static void edf_enqueue(struct task_queue *tq, struct task_struct *task)
{
	/* reset runtime to full */
	task->runtime = task->attr.wcet;

	/* XXX */
	list_add_tail(&task->node, &tq->new);


	if (task->sched->check_sched_attr(&task->attr))
		return;

	if (edf_schedulable(tq, task)) {
		printk("---- NOT SCHEDUL-ABLE---\n");
		return;
	}
#if 0
	/** XXX **/
	if (task->state == TASK_RUN)
		list_move(&task->node, &tq->run);
	else

#endif
#if 1
	list_move_tail(&task->node, &tq->wake);
#endif

}


static ktime edf_timeslice_ns(struct task_struct *task)
{
	return (ktime) (task->runtime);
}

static int edf_check_sched_attr(struct sched_attr *attr)
{
	return 0; /* XXX */
	if (!attr)
		goto error;

	if (attr->policy != SCHED_EDF) {
		pr_err(MSG "attribute policy is %d, expected SCHED_EDF (%d)\n",
			attr->policy, SCHED_EDF);
		return -EINVAL;
	}

	/* need only check WCET, all other times are longer */
	if (attr->wcet < (ktime) tick_get_period_min_ns()) {
		pr_err(MSG "Cannot schedule EDF task with WCET of %llu ns, "
		           "minimum tick duration is %lld\n", attr->wcet,
			   (ktime) tick_get_period_min_ns());
		goto error;
	}

	if (attr->wcet >= attr->period) {
		pr_err(MSG "Cannot schedule EDF task with WCET %u >= "
		           "PERIOD %u!\n", attr->wcet, attr->period);
		goto error;
	}

	if (attr->wcet >= attr->deadline_rel) {
		pr_err(MSG "Cannot schedule EDF task with WCET %llu >= "
		           "DEADLINE %llu !\n", attr->wcet, attr->deadline_rel);
		goto error;
	}

	if (attr->deadline_rel >= attr->period) {
		pr_err(MSG "Cannot schedule EDF task with DEADLINE %llu >= "
		           "PERIOD %llu !\n", attr->deadline_rel, attr->period);
		goto error;
	}


	return 0;
error:

	return -EINVAL;
}

/* called after pick_next() */


ktime edf_task_ready_ns(struct task_queue *tq, ktime now)
{
	int64_t delta;

	int cpu = leon3_cpuid();

	struct task_struct *first;
	struct task_struct *tsk;
	struct task_struct *tmp;
	ktime slice = 12345679123ULL;
	ktime wake = 123456789123ULL;



	list_for_each_entry_safe(first, tmp, &tq->run[cpu], node) {
		if (first->state != TASK_RUN)
			continue;

		slice = first->runtime;
		break;
	}

	list_for_each_entry_safe(tsk, tmp, &tq->run[cpu], node) {
#if 0
		if (tsk->state == TASK_BUSY)
			continue; /*meh */
#endif
		delta = ktime_delta(tsk->wakeup, now);

		if (delta <= 0)
		    continue;

		if (wake > delta)
			wake = delta;
	}

	if (slice > wake)
		slice = wake;

	BUG_ON(slice <= 0);

	return slice;
}


static struct scheduler sched_edf = {
	.policy           = SCHED_EDF,
	.pick_next_task   = edf_pick_next,
	.wake_next_task   = edf_wake_next,
	.enqueue_task     = edf_enqueue,
	.timeslice_ns     = edf_timeslice_ns,
	.task_ready_ns    = edf_task_ready_ns,
	.check_sched_attr = edf_check_sched_attr,
	.sched_priority   = 1,
};



static int sched_edf_init(void)
{
	int i;

	INIT_LIST_HEAD(&sched_edf.tq.new);
	INIT_LIST_HEAD(&sched_edf.tq.wake);
	INIT_LIST_HEAD(&sched_edf.tq.dead);

	for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++)
		INIT_LIST_HEAD(&sched_edf.tq.run[i]);

	sched_register(&sched_edf);

	return 0;
}
postcore_initcall(sched_edf_init);
