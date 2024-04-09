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
#include <kernel/init.h>
#include <kernel/smp.h>

#include <asm/spinlock.h>
#include <asm-generic/irqflags.h>



#define MSG "SCHED_EDF: "

#define UTIL_MAX 0.98 /* XXX should be config option, also should be adaptive depending on RT load */

static struct spinlock edf_spinlock;

extern struct thread_info *current_set[];	/* XXX meh... */


/**
 * @brief lock critical edf section
 */

static void edf_lock(void)
{
	spin_lock_raw(&edf_spinlock);
}


/**
 * @brief unlock critical edf section
 */

static void edf_unlock(void)
{
	spin_unlock(&edf_spinlock);
}


void sched_print_edf_list_internal(struct task_queue *tq, int cpu, ktime now)
{
	char state = 'U';

	struct task_struct *tsk;
	struct task_struct *tmp;

	ktime rt;
	ktime tot;
	ktime wcet;
	ktime avg;
	ktime dead;
	ktime wake;


	printk("\nktime: %lld CPU %d\n", ktime_to_us(now), cpu);
	printk("S\tDeadline\tWakeup\t\tt_rem\ttotal\tslices\tName\t\twcet\tavg(us)\n");
	printk("------------------------------------------------------------------\n");
	list_for_each_entry_safe(tsk, tmp, &tq->run, node) {



		if (tsk->state == TASK_IDLE)
			state = 'I';
		if (tsk->state == TASK_RUN)
			state = 'R';
		if (tsk->state == TASK_BUSY)
			state = 'B';

		if (tsk->slices == 0)
			tsk->slices = 1;

		dead = ktime_to_us(tsk->deadline);
		wake = ktime_to_us(tsk->wakeup);
		rt   = ktime_to_us(tsk->runtime);
		tot  = ktime_to_us(tsk->total);
		wcet = ktime_to_us(tsk->attr.wcet);
		avg  = ktime_to_us(tsk->total/tsk->slices);

		printk("%c\t%lld\t\t%lld\t\t%lld\t%lld\t%d\t%s\t|\t%lld\t%lld\n",
		       state, wake, dead,
		       rt, tot,
		       tsk->slices, tsk->name,
		       wcet,
		       avg);

	}

	printk("\n\n");
}


/**
 * @brief check if an EDF task can still execute given its deadline and
 *        remaining runtime
 *
 *
 * @returns true if can still execute before deadline
 *
 *
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

static inline bool schedule_edf_can_execute(struct task_struct *tsk, int cpu, ktime now)
{
	ktime tick;
	ktime to_deadline;



	/* consider twice the min tick period for overhead */
	tick = tick_get_period_min_ns() << 1;

	if (tsk->runtime <= tick)
		return false;

	to_deadline = ktime_delta(tsk->deadline, now);

	if (ktime_delta(tsk->deadline, now) <= tick)
		return false;


	return true;
}


/**
 * @brief reinitialise a task
 */

static inline void schedule_edf_reinit_task(struct task_struct *tsk, ktime now)
{
	ktime new_wake;



	/* XXX this task never ran, we're in serious trouble */
	if (tsk->runtime == tsk->attr.wcet) {
		printk("T == WCET!! %s\n", tsk->name);
		__asm__ __volatile__ ("ta 0\n\t");
	}


	new_wake = ktime_add(tsk->wakeup, tsk->attr.period);

	/* deadline missed earlier?
	 * XXX need FDIR procedure for this situation: report and wind
	 *     wakeup/deadline forward
	 */

	if (ktime_after(now, new_wake)) {

		printk("%s violated, rt: %lld, next wake: %lld (%lld)\n",
		       tsk->name, tsk->runtime, tsk->wakeup, new_wake);

		sched_print_edf_list_internal(&tsk->sched->tq[tsk->on_cpu],
					      tsk->on_cpu, now);

		/* XXX raise kernel alarm and attempt to recover wakeup */
		__asm__ __volatile__ ("ta 0\n\t");
	}


	if (tsk->flags & TASK_RUN_ONCE) {
		tsk->state = TASK_DEAD;
		return;
	}


	tsk->state = TASK_IDLE;

	tsk->wakeup = new_wake;

	tsk->deadline = ktime_add(tsk->wakeup, tsk->attr.deadline_rel);

	tsk->runtime = tsk->attr.wcet;

	tsk->slices++;
}


static ktime edf_hyperperiod(struct task_queue tq[], int cpu, const struct task_struct *task)
{
       ktime lcm = 1;

       ktime a,b;

       struct task_struct *tsk;
       struct task_struct *tmp;


	lcm = task->attr.period;

       /* argh, need to consider everything */
       list_for_each_entry_safe(tsk, tmp, &tq[cpu].run, node) {

               a = lcm;
               b = tsk->attr.period;

               /* already a multiple? */
               if (a % b == 0)
                       continue;

               while (a != b) {
                       if (a > b)
                               a -= b;
                       else
                               b -= a;
               }

               lcm = lcm * (tsk->attr.period / a);
       }


       /* meh ... */
       list_for_each_entry_safe(tsk, tmp, &tq[cpu].wake, node) {

               a = lcm;
               b = tsk->attr.period;

               /* already a multiple? */
               if (a % b == 0)
                       continue;

               while (a != b) {
                       if (a > b)
                               a -= b;
                       else
                               b -= a;
               }

               lcm = lcm * (tsk->attr.period / a);
       }


       return lcm;
}


/**
 * @brief basic EDFutilisation
 *
 * @param task may be NULL to be excluded
 *
 */

static double edf_utilisation(struct task_queue tq[], int cpu,
			      const struct task_struct *task)
{
	double u = 0.0;

	struct task_struct *t;
	struct task_struct *tmp;


	/* add new task */
	if (task)
		u = (double)  task->attr.wcet / (double)  task->attr.period;

	/* add tasks queued in wakeup */
	list_for_each_entry_safe(t, tmp, &tq[cpu].wake, node)
		u += (double) t->attr.wcet / (double) t->attr.period;

	/* add all running */
	list_for_each_entry_safe(t, tmp, &tq[cpu].run, node)
		u += (double) t->attr.wcet / (double) t->attr.period;

	return u;
}


static int edf_find_cpu(struct task_queue tq[], const struct task_struct *task)
{
	int cpu = -ENODEV;
	int i;
	double u;
	double u_max = 0.0;



	/* XXX need cpu_nr_online() */
	for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++) {

		u = edf_utilisation(tq, i, task);

		if (u > UTIL_MAX)
			continue;

		if (u > u_max) {
			u_max = u;
			cpu = i;
		}
	}

	return cpu;
}

/**
 * @brief returns the longest period task
 */

static struct task_struct *
edf_longest_period_task(struct task_queue tq[], int cpu,
			const struct task_struct *task)
{
	ktime max = 0;

	struct task_struct *t;
	struct task_struct *tmp;

	struct task_struct *t0 = NULL;



	t0 = (struct task_struct *) task;
	max = task->attr.period;

	list_for_each_entry_safe(t, tmp, &tq[cpu].wake, node) {
		if (t->attr.period > max) {
			t0 = t;
			max = t->attr.period;
		}
	}

	list_for_each_entry_safe(t, tmp, &tq[cpu].run, node) {
		if (t->attr.period > max) {
			t0 = t;
			max = t->attr.period;
		}
	}

	return t0;
}

/**
 * @brief performs a more complex slot utilisation test
 *
 * @returns  0 if the new task is schedulable
 */

static int edf_test_slot_utilisation(struct task_queue tq[], int cpu,
				     const struct task_struct *task)
{
	ktime p;
	ktime h;

	ktime uh, ut, f1;
	ktime sh = 0, st = 0;
	ktime stmin = 0x7fffffffffffffULL;
	ktime shmin = 0x7fffffffffffffULL;

	struct task_struct *t0;
	struct task_struct *tsk;
	struct task_struct *tmp;



	t0 = edf_longest_period_task(tq, cpu, task);
	if (!t0)
		return 0;


	p = edf_hyperperiod(tq, cpu, task);

	/* XXX don't know why h=1 would work, needs proper testing */
#if 1
	h = p / t0->attr.period; /* period factor */
#else
	h = 1;
#endif


	/* max available head and tail slots before and after deadline of
	 * longest task
	 */
	uh = h * (t0->attr.deadline_rel - t0->attr.wcet);
	ut = h * (t0->attr.period - t0->attr.deadline_rel);
	f1 = ut/h;


	/* tasks queued in wakeup */
	list_for_each_entry_safe(tsk, tmp, &tq[cpu].wake, node) {

		if (tsk == t0)
			continue;

		if (tsk->attr.deadline_rel <= t0->attr.deadline_rel) {

			/* slots before deadline of T0 */
			sh = h * tsk->attr.wcet * t0->attr.deadline_rel / tsk->attr.period;

			if (sh < shmin)
				shmin = sh;

			if (sh > uh)
				return -1;

			uh = uh - sh;
		}

		/* slots after deadline of T0 */
		st = h * tsk->attr.wcet * f1 / tsk->attr.period;
		if (st < stmin)
			stmin = st;

		if (st > ut)
			return -2;	/* not schedulable in remaining tail */

		ut = ut - st;		/* update tail utilisation */
	}


	/* tasks queued in run */
	list_for_each_entry_safe(tsk, tmp, &tq[cpu].run, node) {

		if (tsk == t0)
			continue;

		if (tsk->attr.deadline_rel <= t0->attr.deadline_rel) {

			/* slots before deadline of T0 */
			sh = h * tsk->attr.wcet * t0->attr.deadline_rel / tsk->attr.period;

			if (sh < shmin)
				shmin = sh;

			if (sh > uh)
				return -1;

			uh = uh - sh;
		}

		/* slots after deadline of T0 */
		st = h * tsk->attr.wcet * f1 / tsk->attr.period;
		if (st < stmin)
			stmin = st;

		if (st > ut)
			return -2;	/* not schedulable in remaining tail */

		ut = ut - st;		/* update tail utilisation */
	}



	if (task != t0) {
		if (task->attr.deadline_rel <= t0->attr.deadline_rel) {

			/* slots before deadline of T0 */
			sh = h * task->attr.wcet * t0->attr.deadline_rel / task->attr.period;

			if (sh < shmin)
				shmin = sh;

			if (sh > uh)
				return -1;

			uh = uh - sh;
		}

		/* slots after deadline of T0 */
		st = h * task->attr.wcet * f1 / task->attr.period;
		if (st < stmin)
			stmin = st;

		if (st > ut)
			return -2;	/* not schedulable in remaining tail */

		ut = ut - st;		/* update tail utilisation */
	}


	return 0;
}


/**
 * @brief EDF schedulability test
 *
 * @returns the cpu to run on if schedulable, -EINVAL otherwise
 *
 *
 * We perform two tests, the first is the very basic
 *
 *        __  WCET_i
 *        \   ------  <= 1
 *        /_   P_i
 *
 * the other one is slightly more complex (with example values):
 *
 *
 * 1) determine task with longest period
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
 * Note: EDF in SMP configurations is not an optimal algorithm, and deadlines
 *	 cannot be guaranteed even for utilisation values just above 1.0
 *	 (Dhall's effect). In order to mitigate this for EDF tasks with no
 *	 CPU affinity set (KTHREAD_CPU_AFFINITY_NONE), we search the per-cpu
 *	 queues until we find one which is below the utilisation limit and
 *	 force the affinity of the task to that particular CPU
 */

static int edf_schedulable(struct task_queue tq[], const struct task_struct *task)
{
	int cpu = -ENODEV;


	if (task->on_cpu == KTHREAD_CPU_AFFINITY_NONE) {

		cpu = edf_find_cpu(tq, task);
		if (cpu < 0)
			return cpu;

	} else {
		cpu = task->on_cpu;
	}



	/* try to locate a CPU which could fit the task
	 *
	 * XXX this needs some rework, also we must consider only
	 * cpus which are actually online
	 */
	if (edf_test_slot_utilisation(tq, cpu, task)) {

		int i;

		for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++) {

			if (i == cpu)
				continue;

			if (edf_utilisation(tq, cpu, task) > UTIL_MAX)
				continue;

			if (edf_test_slot_utilisation(tq, i, task))
				continue;

			return i; /* found one */
		}

	}

	if (edf_utilisation(tq, cpu, task) > UTIL_MAX)
		return -ENODEV;

	/* TODO check utilisation against projected interrupt rate */

	return cpu;
}


/**
 * @brief select the next task to run
 */

static struct task_struct *edf_pick_next(struct task_queue *tq, int cpu,
					 ktime now)
{
	ktime tick;
	ktime delta;

	struct task_struct *tsk;
	struct task_struct *tmp;
	struct task_struct *first;

	struct task_struct *this = current_set[smp_cpu_id()]->task;


	if (list_empty(&tq[cpu].run))
		return NULL;


	/* we use twice the tick period as minimum time to a wakeup */
	tick = (ktime) tick_get_period_min_ns() << 1;

	edf_lock();


	list_for_each_entry_safe(tsk, tmp, &tq[cpu].run, node) {


		/* time to wake up yet? */
		delta = ktime_delta(tsk->wakeup, now);

		/* not yet... */
		if (delta > tick)
			continue;


		/* if it's already running, see if there is time remaining */
		if (tsk->state == TASK_RUN) {

			/* If it has to be reinitialised, always queue it
			 * up at the tail.
			 */
			if (!schedule_edf_can_execute(tsk, cpu, now)) {
				schedule_edf_reinit_task(tsk, now);
				list_move_tail(&tsk->node, &tq[cpu].run);
			}


			/* if our deadline is earlier than the deadline of the
			 * head of the list, we become the new head
			 */

			first = list_first_entry(&tq[cpu].run,
						 struct task_struct, node);

			if (ktime_before (now, tsk->wakeup))
				continue;

			if (ktime_before (tsk->deadline, first->deadline)) {
				tsk->state = TASK_RUN;
				list_move(&tsk->node, &tq[cpu].run);
			}

			continue;
		}

		/* time to wake up */
		if (tsk->state == TASK_IDLE) {

			tsk->state = TASK_RUN;

			/* if our deadline is earlier than the deadline at the
			 * head of the list, move us to top */

			first = list_first_entry(&tq[cpu].run,
						 struct task_struct, node);

			if (first->state != TASK_RUN) {
				list_move(&tsk->node, &tq[cpu].run);
				continue;
			}

			if (ktime_before (tsk->deadline, first->deadline))
				list_move(&tsk->node, &tq[cpu].run);

			continue;
		}

		if (tsk->state == TASK_DEAD){ /* XXX need other mechanism */
			if (tsk == this)
				continue;

			list_del(&tsk->node);
			kthread_free(tsk);
			continue;
		}
	}


	first = list_first_entry(&tq[cpu].run, struct task_struct, node);


	edf_unlock();

	if (first->state == TASK_RUN) {
		first->state = TASK_BUSY;
		return first;
	}

	return NULL;
}


/**
 * @brief verify that a task is in the wake queue
 */

static int edf_task_in_wake_queue(struct task_struct *task,
				  struct task_queue tq[],
				  int cpu)
{
	int found = 0;

	struct task_struct *t;
	struct task_struct *tmp;


	list_for_each_entry_safe(t, tmp, &tq[cpu].wake, node) {

		if (t != task)
			continue;

		found = 1;
		break;
	}

	return found;
}


/**
 * @brief determine the earliest sensible wakeup for a periodic task given
 *	  the current run queue
 */

static ktime edf_get_earliest_wakeup(struct task_queue tq[],
				     int cpu, ktime now)
{
	ktime max  = 0;
	ktime wakeup = now;

	struct task_struct *t;
	struct task_struct *tmp;
	struct task_struct *after = NULL;


	list_for_each_entry_safe(t, tmp, &tq[cpu].run, node) {

		if (t->state == TASK_DEAD)
			continue;

		if (t->flags & TASK_RUN_ONCE)
			continue;

		if (max > t->attr.period)
			continue;

		max   = t->attr.period;
		after = t;
	}

	if (after)
		wakeup = ktime_add(after->wakeup, after->attr.period);

	return wakeup;
}


/**
 * @brief sort run queue in order of urgency of wakeup
 */

static void edf_sort_queue_by_urgency(struct task_queue tq[],
				      int cpu, ktime now)
{
	struct task_struct *t;
	struct task_struct *tmp;
	struct task_struct *first;

	list_for_each_entry_safe(t, tmp, &tq[cpu].run, node) {

		if (t->flags & TASK_RUN_ONCE)
			continue;

		if (t->state == TASK_DEAD)
			continue;

		first = list_first_entry(&tq[cpu].run, struct task_struct, node);

		if (ktime_before (t->wakeup, now)) {

			ktime latest_wake;

			latest_wake = ktime_delta(t->deadline, t->runtime);

			if (ktime_before(latest_wake, first->deadline))
				list_move(&t->node, &tq[cpu].run);

		}
	}
}


/**
 * @brief if possible, adjust the wakeup for a given periodic task
 *
 * @param last the previous best estimate of the wakeup time
 */

static ktime edf_get_best_wakeup(struct task_struct *task,
				 ktime wakeup,
				 struct task_queue tq[],
				 int cpu, ktime now)
{
	struct task_struct *t;
	struct task_struct *tmp;


	/* locate the best position withing the run queue's hyperperiod
	 * for the task-to-wake
	 */
	list_for_each_entry_safe(t, tmp, &tq[cpu].run, node) {

		ktime delta;

		if (t->flags & TASK_RUN_ONCE)
			continue;

		if (t->state != TASK_IDLE)
			continue;

		if (ktime_before (t->wakeup, now))
			continue;

		/* if the relative deadline of our task-to-wake can fit in
		 * between the unused timeslices of a running task, insert
		 * it after its next wakeup
		 */

		delta = ktime_delta(t->deadline, t->wakeup);

		if (task->attr.deadline_rel < delta) {
			wakeup = t->wakeup;
			break;
		}

		if (task->attr.wcet < delta) {
			wakeup = t->deadline;
			break;
		}
	}

	return wakeup;
}


/**
 * @brief wake up a task by inserting in into the run queue
 */

static int edf_wake(struct task_struct *task, ktime now)
{
	int cpu;

	ktime wakeup;

	unsigned long flags;

	struct task_queue *tq;


	if (!task)
		return -EINVAL;

	if (task->attr.policy != SCHED_EDF)
		return -EINVAL;


	tq  = task->sched->tq;
	cpu = task->on_cpu;

	if (cpu == KTHREAD_CPU_AFFINITY_NONE)
		return -EINVAL;

	if (list_empty(&tq[cpu].wake))
		return -EINVAL;

	if (!edf_task_in_wake_queue(task, tq, cpu))
		return -EINVAL;


	wakeup = now;

	flags = arch_local_irq_save();
	edf_lock();

	/* if this is first task or a non-periodic task, run it asap, otherwise
	 * we try to find a good insertion point
	 */
	if (!list_empty(&tq[cpu].run) || !(task->flags & TASK_RUN_ONCE)) {
		wakeup = edf_get_earliest_wakeup(tq, cpu, now);
		edf_sort_queue_by_urgency(tq, cpu, now);
		wakeup = edf_get_best_wakeup(task, wakeup, tq, cpu, now);
	}


	/* shift wakeup by minimum tick period */
	wakeup         = ktime_add(wakeup, tick_get_period_min_ns());
	task->wakeup   = ktime_add(wakeup, task->attr.period);
	task->deadline = ktime_add(task->wakeup, task->attr.deadline_rel);

	/* reset runtime to full */
	task->runtime = task->attr.wcet;
	task->state   = TASK_IDLE;

	list_move_tail(&task->node, &tq[cpu].run);

	edf_unlock();
	arch_local_irq_restore(flags);

	return 0;
}


/**
 * @brief enqueue a task
 *
 * @returns 0 if the task can be scheduled, ENOSCHED otherwise
 */

static int edf_enqueue(struct task_struct *task)
{
	int cpu;
	unsigned long flags;

	struct task_queue *tq = task->sched->tq;


	if (!task->attr.period) {
		task->flags |= TASK_RUN_ONCE;
		task->attr.period = task->attr.deadline_rel;
	} else
		task->flags &= ~TASK_RUN_ONCE;

	flags = arch_local_irq_save();
	edf_lock();

	cpu = edf_schedulable(tq, task);
	if (cpu < 0) {
		edf_unlock();
		arch_local_irq_restore(flags);
		return -ENOSCHED;
	}

	task->on_cpu = cpu;

	list_add_tail(&task->node, &tq[cpu].wake);

	edf_unlock();
	arch_local_irq_restore(flags);


	return 0;
}


/**
 * @brief get remaining time slice for a given task
 */

static ktime edf_timeslice_ns(struct task_struct *task)
{
	return (ktime) (task->runtime);
}


/**
 * @brief verify scheduling attributes
 */

static int edf_check_sched_attr(struct sched_attr *attr)
{
	ktime tick_min;


	if (!attr)
		goto error;

	tick_min = (ktime) tick_get_period_min_ns();

	if (attr->policy != SCHED_EDF) {
		pr_err(MSG "attribute policy is %d, expected SCHED_EDF (%d)\n",
			attr->policy, SCHED_EDF);
		return -EINVAL;
	}

	if (attr->wcet < tick_min) {
		pr_err(MSG "Cannot schedule EDF task with WCET of %lld ns, "
		           "minimum tick duration is %lld\n", attr->wcet,
			   tick_min);
		goto error;
	}

	if (ktime_delta(attr->deadline_rel, attr->wcet) < tick_min) {
		pr_err(MSG "Cannot schedule EDF task with WCET-deadline delta "
		           "of %lld ns, minimum tick duration is %lld\n",
			   ktime_delta(attr->deadline_rel, attr->wcet),
			   tick_min);
		goto error;
	}


	if (attr->period > 0) {

		if (attr->wcet >= attr->period) {
			pr_err(MSG "Cannot schedule EDF task with WCET %lld >= "
			       "PERIOD %lld!\n", attr->wcet, attr->period);
			goto error;
		}
		if (attr->deadline_rel >= attr->period) {
			pr_err(MSG "Cannot schedule EDF task with DEADLINE %lld >= "
			       "PERIOD %lld!\n", attr->deadline_rel, attr->period);
			goto error;
		}


		/* this is only relevant for periodic tasks */
		if (ktime_delta(attr->period, attr->deadline_rel) < tick_min) {
			pr_err(MSG "Cannot schedule EDF task with deadline-period delta "
			       "of %llu ns, minimum tick duration is %lld\n",
			       ktime_delta(attr->period, attr->deadline_rel),
			       tick_min);
			goto error;
		}
	}


	if (attr->wcet >= attr->deadline_rel) {
		pr_err(MSG "Cannot schedule EDF task with WCET %lld >= "
		           "DEADLINE %lld !\n", attr->wcet, attr->deadline_rel);
		goto error;
	}



	return 0;
error:

	return -EINVAL;
}


/**
 * @brief returns the next ktime when a task will become ready
 */

ktime edf_task_ready_ns(struct task_queue *tq, int cpu, ktime now)
{
	ktime tick;
	ktime delta;
	ktime ready = (unsigned int) ~0 >> 1;

	struct task_struct *tsk;
	struct task_struct *tmp;



	/* we use twice the tick period as minimum time to a wakeup */
	tick = (ktime) tick_get_period_min_ns() << 1;

	list_for_each_entry_safe(tsk, tmp, &tq[cpu].run, node) {

		if (tsk->state == TASK_BUSY)
			continue;

		delta = ktime_delta(tsk->wakeup, now);

		if (delta <= tick)
			continue;

		if (ready > delta)
			ready = delta;
	}


	return ready;
}


struct scheduler sched_edf = {
	.policy           = SCHED_EDF,
	.pick_next_task   = edf_pick_next,
	.wake_task        = edf_wake,
	.enqueue_task     = edf_enqueue,
	.timeslice_ns     = edf_timeslice_ns,
	.task_ready_ns    = edf_task_ready_ns,
	.check_sched_attr = edf_check_sched_attr,
	.priority         = SCHED_PRIORITY_EDF,
};



static int sched_edf_init(void)
{
	int i;


	for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++) {
		INIT_LIST_HEAD(&sched_edf.tq[i].wake);
		INIT_LIST_HEAD(&sched_edf.tq[i].run);
		INIT_LIST_HEAD(&sched_edf.tq[i].dead);
	}

	sched_register(&sched_edf);

	return 0;
}
postcore_initcall(sched_edf_init);
