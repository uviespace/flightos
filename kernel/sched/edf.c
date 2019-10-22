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

void sched_print_edf_list_internal(struct task_queue *tq, int cpu, ktime now)
{
//	ktime now;
	char state = 'U';
	int64_t rel_deadline;
	int64_t rel_wait;

	struct task_struct *tsk;
	struct task_struct *tmp;


	ktime prev = 0;
	ktime prevd = 0;

	printk("\nktime: %lld\n", ktime_to_ms(now));
	printk("S\tDeadline\tWakeup\t\tdelta W\tdelta P\tt_rem\ttotal\tslices\tName\twcet\tavg\n");
	printk("---------------------------------------------------------------------------------------------------------------------------------\n");
	list_for_each_entry_safe(tsk, tmp, &tq->run, node) {

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
		if (tsk->state == TASK_BUSY)
			state = 'B';

		if (tsk->slices == 0)
			tsk->slices = 1;

		printk("%c\t%lld\t\t%lld\t\t%lld\t%lld\t%lld\t%lld\t%d\t%s\t|\t%lld\t%lld\n",
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
 * @brief check if an EDF task can still execute given its deadline and
 *        remaining runtime
 *
 *
 * @returns true if can still execute before deadline
 */

static inline bool schedule_edf_can_execute(struct task_struct *tsk, int cpu, ktime now)
{
	int64_t to_deadline;




	/* should to consider twice the min tick period for overhead */
	if (tsk->runtime <= (tick_get_period_min_ns() << 1))
		return false;

	to_deadline = ktime_delta(tsk->deadline, now);

	/* should to consider twice the min tick period for overhead */
	if (to_deadline <= (tick_get_period_min_ns() << 1))
		return false;

	return true;
}


static inline void schedule_edf_reinit_task(struct task_struct *tsk, ktime now)
{
	tsk->state = TASK_IDLE;

	tsk->wakeup = ktime_add(tsk->wakeup, tsk->attr.period);
#if 1
	/* need FDIR procedure for this situation */
	if (ktime_after(now, tsk->wakeup)){ /* deadline missed earlier? */
		printk("%s delta %lld\n",tsk->name, ktime_us_delta(tsk->wakeup, now));
		printk("%s violated, %lld %lld, dead %lld wake %lld now %lld start %lld\n", tsk->name,
		       tsk->runtime, ktime_us_delta(tsk->deadline, now),
		       tsk->deadline, tsk->wakeup, now, tsk->exec_start);
		sched_print_edf_list_internal(&tsk->sched->tq[tsk->on_cpu], tsk->on_cpu, now);
		BUG();
	}
#endif

	tsk->deadline = ktime_add(tsk->wakeup, tsk->attr.deadline_rel);

	tsk->runtime = tsk->attr.wcet;

	tsk->slices++;
}


#include <kernel/init.h>

#define MSG "SCHED_EDF: "


static ktime edf_hyperperiod(struct task_queue tq[], int cpu, const struct task_struct *task)
{
       ktime lcm = 1;

       ktime a,b;

       struct task_struct *t0 = NULL;
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
 * @brief EDF schedulability test
 *
 * @returns the cpu to run on if schedulable, -EINVAL otherwise
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
#define UTIL_MAX 0.8
static int edf_schedulable(struct task_queue tq[], const struct task_struct *task)
{
	struct task_struct *tsk = NULL;
	struct task_struct *tmp;

	int cpu = -EINVAL;

	double u = 0.0;	/* utilisation */





	if (task->on_cpu == KTHREAD_CPU_AFFINITY_NONE) {
		int i;
		double util_max = 0.0;
		double util;

		for (i = 0; i < 2; i++) {
			/* XXX look for best fit */
			double util;
			/* add new task */
			util= (double) (int32_t) task->attr.wcet / (double) (int32_t) task->attr.period;


			/* add tasks queued in wakeup */
			if (!list_empty(&tq[i].wake)) {
				list_for_each_entry_safe(tsk, tmp, &tq[i].wake, node) {
					util += (double) (int32_t) tsk->attr.wcet / (double) (int32_t) tsk->attr.period;
				}

			}

			/* add all running */
			if (!list_empty(&tq[i].run)) {
				list_for_each_entry_safe(tsk, tmp, &tq[i].run, node)
					util += (double) (int32_t) tsk->attr.wcet / (double) (int32_t) tsk->attr.period;
			}


			printk("UTIL %g\n", util);
			if (util > UTIL_MAX)
				continue;

			if (util > util_max) {
				util_max = util;
				cpu = i;
			}


		}
		if (cpu == -EINVAL) { 
			printk("---- WILL NOT FIT ----\n");
			return -EINVAL;
		}

		printk("best fit is %d\n", cpu);
	} else {
		cpu = task->on_cpu;
	}


retry:
	/******/
if (1)
{
	int nogo = 0;
	printk("\n\n\n");
	ktime p;
	ktime h;
	ktime max = 0;

	ktime uh, ut, f1;
	ktime sh = 0, st = 0;
        ktime stmin = 0x7fffffffffffffULL;
        ktime shmin = 0x7fffffffffffffULL;

	struct task_struct *t0;

	printk("hyper? %s %lld\n", task->name, ktime_to_ms(task->attr.period));
	p = edf_hyperperiod(tq, cpu, task);
	printk("hyper %llu\n", ktime_to_ms(p));


	/* new */
	t0 = (struct task_struct *) task;
	max = task->attr.period;

	/* add tasks queued in wakeup */
	if (!list_empty(&tq[cpu].wake)) {
		list_for_each_entry_safe(tsk, tmp, &tq[cpu].wake, node) {
			 if (tsk->attr.period > max) {
				 t0 = tsk;
				 max = tsk->attr.period;
			 }
		}
	}

	/* add tasks queued in run */
	if (!list_empty(&tq[cpu].run)) {
		list_for_each_entry_safe(tsk, tmp, &tq[cpu].run, node) {
			 if (tsk->attr.period > max) {
				 t0 = tsk;
				 max = tsk->attr.period;
			 }
		}
	}

	printk("t0: %s (cpu %d)\n", t0->name, cpu);
	h = p / t0->attr.period;

	printk("Period factor %lld, duration %lld actual period: %lld\n", h, ktime_to_ms(p), ktime_to_ms(t0->attr.period));


	uh = h * (t0->attr.deadline_rel - t0->attr.wcet);
	ut = h * (t0->attr.period - t0->attr.deadline_rel);
	f1 = ut/h;

	printk("max UH: %lld, UT: %lld\n", ktime_to_ms(uh), ktime_to_ms(ut));


#if 0
	/* XXX already implicitly subtracted! */
	/* subtract longest period thread from head, its slices must always
	 * be used before the deadline
	 */
	sh = h * t0->attr.wcet * t0->attr.deadline_rel / t0->attr.period;

	if (sh < shmin)
		shmin = sh;

	uh = uh - sh;
	printk("%s UH: %lld, UT: %lld\n", t0->name, ktime_to_ms(uh), ktime_to_ms(ut));
	printk("%s SH: %lld, ST: %lld\n", t0->name, ktime_to_ms(sh), ktime_to_ms(st));
#endif


	/* tasks queued in wakeup */
	if (!list_empty(&tq[cpu].wake)) {
		list_for_each_entry_safe(tsk, tmp, &tq[cpu].wake, node) {

			if (tsk == t0)
				continue;

			printk("tsk wake: %s\n", tsk->name);
			if (tsk->attr.deadline_rel <= t0->attr.deadline_rel) {

				/* slots before deadline of  T0 */
				sh = h * tsk->attr.wcet * t0->attr.deadline_rel / tsk->attr.period;
				if (sh < shmin)
					shmin = sh;
				if (sh > uh) {
					printk("WARN: NOT SCHEDULABLE in head: %s\n", tsk->name);
				nogo = 1;
				}
				uh = uh - sh;
			}

			/* slots after deadline of T0 */
			st = h * tsk->attr.wcet * f1 / tsk->attr.period;
			if (st < stmin)
				stmin = st;

			if (st > ut) {
				printk("WARN: NOT SCHEDULABLE in tail: %s\n", tsk->name);
				nogo = 1;
			}

			ut = ut - st;

			printk("w %s UH: %lld, UT: %lld\n", tsk->name, ktime_to_ms(uh), ktime_to_ms(ut));

			printk("w %s SH: %lld, ST: %lld\n", tsk->name, ktime_to_ms(sh), ktime_to_ms(st));

		}
	}


	/* tasks queued in run */
	if (!list_empty(&tq[cpu].run)) {
		list_for_each_entry_safe(tsk, tmp, &tq[cpu].run, node) {

			if (tsk == t0)
				continue;

			printk("tsk run: %s\n", tsk->name);
			if (tsk->attr.deadline_rel <= t0->attr.deadline_rel) {

				/* slots before deadline of  T0 */
				sh = h * tsk->attr.wcet * t0->attr.deadline_rel / tsk->attr.period;
				if (sh < shmin)
					shmin = sh;
				if (sh > uh) {
					printk("WARN: NOT SCHEDULABLE in head: %s\n", tsk->name);
				nogo = 1;
				}
				uh = uh - sh;
			}

			/* slots after deadline of T0 */
			st = h * tsk->attr.wcet * f1 / tsk->attr.period;
			if (st < stmin)
				stmin = st;

			if (st > ut) {
				printk("WARN: NOT SCHEDULABLE in tail: %s\n", tsk->name);
				nogo = 1;
			}

			ut = ut - st;

			printk("w %s UH: %lld, UT: %lld\n", tsk->name, ktime_to_ms(uh), ktime_to_ms(ut));

			printk("w %s SH: %lld, ST: %lld\n", tsk->name, ktime_to_ms(sh), ktime_to_ms(st));

		}
	}





	if (task != t0) {

		if (task->attr.deadline_rel <= t0->attr.deadline_rel) {
			printk("task: %s\n", task->name);

			/* slots before deadline of  T0 */
			sh = h * task->attr.wcet * t0->attr.deadline_rel / task->attr.period;
			if (sh < shmin)
				shmin = sh;
			if (sh > uh) {
				printk("xWARN: NOT SCHEDULABLE in head: %s\n", task->name);
				nogo = 1;
			}
			uh = uh - sh;
		}

		/* slots after deadline of T0 */
		st = h * task->attr.wcet * f1 / task->attr.period;
		if (st < stmin)
			stmin = st;

		if (st > ut) {
			printk("xWARN: NOT SCHEDULABLE in tail: %s\n", task->name);
			nogo = 1;
		}

		ut = ut - st;

		printk("x %s UH: %lld, UT: %lld\n", task->name, ktime_to_ms(uh), ktime_to_ms(ut));

		printk("x %s SH: %lld, ST: %lld\n", task->name, ktime_to_ms(sh), ktime_to_ms(st));

	}

	if (nogo == 1) {
		if (cpu == 0) {
			cpu = 1;
			printk("RETRY\n");
			goto retry;
		} else {
			printk("RETURN: I am NOT schedul-ableh: %f ", u);
			return -EINVAL;
		}
	}


	printk("\n\n\n");
}
	/*******/




	/* add new task */
	u += (double) (int32_t) task->attr.wcet / (double) (int32_t) task->attr.period;



	/* add tasks queued in wakeup */
	if (!list_empty(&tq[cpu].wake)) {
		list_for_each_entry_safe(tsk, tmp, &tq[cpu].wake, node) {
			u += (double) (int32_t) tsk->attr.wcet / (double) (int32_t) tsk->attr.period;
		}

	}

	/* add all running */
	if (!list_empty(&tq[cpu].run)) {
		list_for_each_entry_safe(tsk, tmp, &tq[cpu].run, node)
			u += (double) (int32_t) tsk->attr.wcet / (double) (int32_t) tsk->attr.period;
	}

	if (u > UTIL_MAX) {
		printk("I am NOT schedul-ableh: %f ", u);
		BUG();
		return -EINVAL;
		printk("changed task mode to RR\n", u);
	}

	printk("Utilisation: %g\n---\n", u);


	/* TODO check against projected interrupt rate, we really need a limit
	 * there */

	return cpu;
}

 void kthread_lock(void);
 void kthread_unlock(void);

static struct task_struct *edf_pick_next(struct task_queue *tq, int cpu,
					 ktime now)
{
	int64_t delta;

	struct task_struct *tsk;
	struct task_struct *tmp;
	struct task_struct *first;

	static int cnt;

	if (list_empty(&tq[cpu].run))
		return NULL;

	kthread_lock();
	list_for_each_entry_safe(tsk, tmp, &tq[cpu].run, node) {

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
			if (!schedule_edf_can_execute(tsk, cpu, now)) {
				schedule_edf_reinit_task(tsk, now);

				/* always queue it up at the tail */
				list_move_tail(&tsk->node, &tq[cpu].run);
			}


			/* if our deadline is earlier than the deadline of the
			 * head of the list, we become the new head
			 */

			first = list_first_entry(&tq[cpu].run, struct task_struct, node);

			if (ktime_before (tsk->wakeup, now)) {
				if (ktime_before (tsk->deadline - tsk->runtime, first->deadline)) {
					tsk->state = TASK_RUN;
					list_move(&tsk->node, &tq[cpu].run);
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

			first = list_first_entry(&tq[cpu].run, struct task_struct, node);

			list_move(&tsk->node, &tq[cpu].run);

			if (ktime_before (tsk->deadline - tsk->runtime, first->deadline))
				list_move(&tsk->node, &tq[cpu].run);

			continue;
		}

	}

	/* XXX argh, need cpu affinity run lists */
	list_for_each_entry_safe(tsk, tmp, &tq[cpu].run, node) {

		if (tsk->state == TASK_RUN) {

			tsk->state = TASK_BUSY;
			kthread_unlock();
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
	kthread_unlock();

	return NULL;
}


static void edf_wake_next(struct task_queue *tq, int cpu, ktime now)
{
	ktime last;

	struct task_struct *tmp;
	struct task_struct *task;
	struct task_struct *first = NULL;
	struct task_struct *t;
	struct task_struct *prev = NULL;


	ktime max = 0;


	struct task_struct *after = NULL;

	ktime wake;



	if (list_empty(&tq[cpu].wake))
		return;

	kthread_lock();
	last = now;


	list_for_each_entry_safe(task, tmp, &tq->run, node) {

		if (max > task->attr.period)
			continue;

		max = task->attr.period;
		after = task;
	}


	if (after) {
		last = ktime_add(after->wakeup, after->attr.period);
	}


#if 1 /* better */
	task = list_first_entry(&tq[cpu].wake, struct task_struct, node);


	BUG_ON(task->on_cpu == KTHREAD_CPU_AFFINITY_NONE);

	if (!list_empty(&tq[cpu].run)) {

		/* reorder */

		list_for_each_entry_safe(t, tmp, &tq[cpu].run, node) {
			first = list_first_entry(&tq[cpu].run, struct task_struct, node);
			if (ktime_before (t->wakeup, now)) {
				if (ktime_before (t->deadline - t->runtime, first->deadline)) {
					list_move(&t->node, &tq[cpu].run);
				}
			}
		}

		list_for_each_entry_safe(t, tmp, &tq[cpu].run, node) {

			if (t->state != TASK_IDLE)
				continue;

			if (ktime_before (t->wakeup, now))
				continue;

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
	}
#endif

	task->state = TASK_IDLE;

	/* initially furthest deadline as wakeup */
	last  = ktime_add(last, 5000000000ULL);
	task->wakeup     = ktime_add(last, task->attr.period);
	task->deadline   = ktime_add(task->wakeup, task->attr.deadline_rel);


//	printk("%s now %lld, last %lld, wake at %lld dead at %lld\n", task->name, now, last, task->wakeup, task->deadline);

	list_move_tail(&task->node, &tq[cpu].run);
	kthread_unlock();
}




static void edf_enqueue(struct task_queue tq[], struct task_struct *task)
{
	int cpu;


	/* reset runtime to full */
	task->runtime = task->attr.wcet;


	if (task->sched->check_sched_attr(&task->attr))
		return;

	cpu = edf_schedulable(tq, task);

	if (cpu < 0) {
		printk("---- NOT SCHEDUL-ABLE---\n");
		return;
	}
	task->on_cpu = cpu;
#if 0
	/** XXX **/
	if (task->state == TASK_RUN)
		list_move(&task->node, &tq->run);
	else

#endif
#if 1
	list_add_tail(&task->node, &tq[cpu].wake);
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


ktime edf_task_ready_ns(struct task_queue *tq, int cpu, ktime now)
{
	int64_t delta;

	struct task_struct *first;
	struct task_struct *tsk;
	struct task_struct *tmp;
	ktime slice = 12345679123ULL;
	ktime wake = 123456789123ULL;



	list_for_each_entry_safe(first, tmp, &tq[cpu].run, node) {
		if (first->state != TASK_RUN)
			continue;

		slice = first->runtime;
		break;
	}

	list_for_each_entry_safe(tsk, tmp, &tq[cpu].run, node) {
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


	for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++) {
		INIT_LIST_HEAD(&sched_edf.tq[i].new);
		INIT_LIST_HEAD(&sched_edf.tq[i].wake);
		INIT_LIST_HEAD(&sched_edf.tq[i].run);
		INIT_LIST_HEAD(&sched_edf.tq[i].dead);
	}

	sched_register(&sched_edf);

	return 0;
}
postcore_initcall(sched_edf_init);
