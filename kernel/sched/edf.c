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

#include <generated/autoconf.h> /* XXX need common CPU include */


#define MSG "SCHED_EDF: "

#define UTIL_MAX 0.98 /* XXX should be config option, also should be adaptive depending on RT load */


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


	printk("\nktime: %lld CPU %d\n", ktime_to_ms(now), cpu);
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


#include <asm/spinlock.h>
static struct spinlock edf_spinlock;


/**
 * @brief lock critical rr section
 */

 void edf_lock(void)
{
	return;
	spin_lock_raw(&edf_spinlock);
}


/**
 * @brief unlock critical rr section
 */

void edf_unlock(void)
{
	return;
	spin_unlock(&edf_spinlock);
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
	if (ktime_delta(tsk->deadline, now) <= (tick_get_period_min_ns() << 1))
		return false;


	return true;
}


static inline void schedule_edf_reinit_task(struct task_struct *tsk, ktime now)
{
	ktime new_wake;

	if (tsk->runtime == tsk->attr.wcet) {
		printk("T == WCET!! %s\n", tsk->name);
		__asm__ __volatile__ ("ta 0\n\t");
	}


	new_wake = ktime_add(tsk->wakeup, tsk->attr.period);
#if 1
	/* need FDIR procedure for this situation: report and wind
	 * wakeup/deadline forward */

	if (ktime_after(now, new_wake)){ /* deadline missed earlier? */
		printk("%s violated, rt: %lld, next wake: %lld (%lld)\n", tsk->name,
		       tsk->runtime, tsk->wakeup, new_wake);
		sched_print_edf_list_internal(&tsk->sched->tq[tsk->on_cpu], tsk->on_cpu, now);
		__asm__ __volatile__ ("ta 0\n\t");

		/* XXX raise kernel alarm and attempt to recover wakeup */
		BUG();
	}
#endif
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


static int edf_schedulable(struct task_queue tq[], const struct task_struct *task)
{
	struct task_struct *tsk = NULL;
	struct task_struct *tmp;

	int cpu = -EINVAL;

	double u = 0.0;	/* utilisation */




	if (task->on_cpu == KTHREAD_CPU_AFFINITY_NONE) {
		int i;
		double util_max = 0.0;

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


		//	printk("UTIL %g\n", util);
			if (util > UTIL_MAX)
				continue;

			if (util > util_max) {
				util_max = util;
				cpu = i;
			}


		}
		if (cpu == -EINVAL) {
	//		printk("---- WILL NOT FIT ----\n");
			return -EINVAL;
		}

	//	printk("best fit is %d\n", cpu);
	} else {
		cpu = task->on_cpu;
	}


retry:
	/******/
if (1)
{
	int nogo = 0;
	//printk("\n\n\n");
	ktime p;
	ktime h;
	ktime max = 0;

	ktime uh, ut, f1;
	ktime sh = 0, st = 0;
        ktime stmin = 0x7fffffffffffffULL;
        ktime shmin = 0x7fffffffffffffULL;

	struct task_struct *t0;

	//printk("hyper? %s %lld\n", task->name, ktime_to_ms(task->attr.period));
	p = edf_hyperperiod(tq, cpu, task);
	//printk("hyper %llu\n", ktime_to_ms(p));


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

	//printk("t0: %s (cpu %d)\n", t0->name, cpu);
	h = p / t0->attr.period;

	h = 1;
	//printk("Period factor %lld, duration %lld actual period: %lld\n", h, ktime_to_ms(p), ktime_to_ms(t0->attr.period));


	uh = h * (t0->attr.deadline_rel - t0->attr.wcet);
	ut = h * (t0->attr.period - t0->attr.deadline_rel);
	f1 = ut/h;

	//printk("max UH: %lld, UT: %lld\n", ktime_to_ms(uh), ktime_to_ms(ut));


	/* tasks queued in wakeup */
	if (!list_empty(&tq[cpu].wake)) {
		list_for_each_entry_safe(tsk, tmp, &tq[cpu].wake, node) {

			if (tsk == t0)
				continue;

			//printk("tsk wake: %s\n", tsk->name);
			if (tsk->attr.deadline_rel <= t0->attr.deadline_rel) {

				/* slots before deadline of  T0 */
				sh = h * tsk->attr.wcet * t0->attr.deadline_rel / tsk->attr.period;
				if (sh < shmin)
					shmin = sh;
				if (sh > uh) {
					//printk("WARN: NOT SCHEDULABLE in head: %s\n", tsk->name);
				nogo = 1;
				}
				uh = uh - sh;
			}

			/* slots after deadline of T0 */
			st = h * tsk->attr.wcet * f1 / tsk->attr.period;
			if (st < stmin)
				stmin = st;

			if (st > ut) {
				//printk("WARN: NOT SCHEDULABLE in tail: %s\n", tsk->name);
				nogo = 1;
			}

			ut = ut - st;

			//printk("w %s UH: %lld, UT: %lld\n", tsk->name, ktime_to_ms(uh), ktime_to_ms(ut));

			//printk("w %s SH: %lld, ST: %lld\n", tsk->name, ktime_to_ms(sh), ktime_to_ms(st));

		}
	}


	/* tasks queued in run */
	if (!list_empty(&tq[cpu].run)) {
		list_for_each_entry_safe(tsk, tmp, &tq[cpu].run, node) {

			if (tsk == t0)
				continue;

			//printk("tsk run: %s\n", tsk->name);
			if (tsk->attr.deadline_rel <= t0->attr.deadline_rel) {

				/* slots before deadline of  T0 */
				sh = h * tsk->attr.wcet * t0->attr.deadline_rel / tsk->attr.period;
				if (sh < shmin)
					shmin = sh;
				if (sh > uh) {
					//printk("WARN: NOT SCHEDULABLE in head: %s\n", tsk->name);
				nogo = 1;
				}
				uh = uh - sh;
			}

			/* slots after deadline of T0 */
			st = h * tsk->attr.wcet * f1 / tsk->attr.period;
			if (st < stmin)
				stmin = st;

			if (st > ut) {
				//printk("WARN: NOT SCHEDULABLE in tail: %s\n", tsk->name);
				nogo = 1;
			}

			ut = ut - st;

			//printk("w %s UH: %lld, UT: %lld\n", tsk->name, ktime_to_ms(uh), ktime_to_ms(ut));

			//printk("w %s SH: %lld, ST: %lld\n", tsk->name, ktime_to_ms(sh), ktime_to_ms(st));

		}
	}





	if (task != t0) {

		if (task->attr.deadline_rel <= t0->attr.deadline_rel) {
			//printk("task: %s\n", task->name);

			/* slots before deadline of  T0 */
			sh = h * task->attr.wcet * t0->attr.deadline_rel / task->attr.period;
			if (sh < shmin)
				shmin = sh;
			if (sh > uh) {
				//printk("xWARN: NOT SCHEDULABLE in head: %s\n", task->name);
				nogo = 1;
			}
			uh = uh - sh;
		}

		/* slots after deadline of T0 */
		st = h * task->attr.wcet * f1 / task->attr.period;
		if (st < stmin)
			stmin = st;

		if (st > ut) {
			//printk("xWARN: NOT SCHEDULABLE in tail: %s\n", task->name);
			nogo = 1;
		}

		ut = ut - st;

		//printk("x %s UH: %lld, UT: %lld\n", task->name, ktime_to_ms(uh), ktime_to_ms(ut));

		//printk("x %s SH: %lld, ST: %lld\n", task->name, ktime_to_ms(sh), ktime_to_ms(st));

	}

	if (nogo == 1) {
		if (cpu == 0) {
			cpu = 1;
			//printk("RETRY\n");
			goto retry;
		} else {
			//printk("RETURN: I am NOT schedul-ableh: %f ", u);
			return -EINVAL;
		}
	}


	//printk("\n\n\n");
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
//		printk("I am NOT schedul-ableh: %f ", u);
		BUG();
		return -EINVAL;
		printk("changed task mode to RR\n", u);
	}

//	printk("Utilisation: %g CPU %d\n", u, cpu);


	/* TODO check against projected interrupt rate, we really need a limit
	 * there */

	return cpu;
}


static struct task_struct *edf_pick_next(struct task_queue *tq, int cpu,
					 ktime now)
{
	int64_t delta;

	struct task_struct *tsk;
	struct task_struct *tmp;
	struct task_struct *first;



	if (list_empty(&tq[cpu].run))
		return NULL;

	edf_lock();


	/* XXX need to lock run list for wakeup() */

	list_for_each_entry_safe(tsk, tmp, &tq[cpu].run, node) {


		/* time to wake up yet? */
		delta = ktime_delta(tsk->wakeup, now);

		/* not yet XXX min period to variable */
		if (delta > (tick_get_period_min_ns() << 1))
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
			list_del(&tsk->node);
			kfree(tsk->stack);
			kfree(tsk->name);
			kfree(tsk);
			continue;
		}



	}


	first = list_first_entry(&tq[cpu].run, struct task_struct, node);
	edf_unlock();
	if (first->state == TASK_RUN)
		return first;

	return NULL;
}



#include <asm-generic/irqflags.h>
static void edf_wake_next(struct task_queue *tq, int cpu, ktime now)
{
	ktime last;

	struct task_struct *tmp;
	struct task_struct *task;
	struct task_struct *first = NULL;
	struct task_struct *t;


	ktime max = 0;


	struct task_struct *after = NULL;


	if (list_empty(&tq[cpu].wake))
		return;

	edf_lock();
	last = now;

	/* no period, run it asap */
	task = list_first_entry(&tq[cpu].wake, struct task_struct, node);
	if (task->flags & TASK_RUN_ONCE)
		goto insert;


	list_for_each_entry_safe(task, tmp, &tq->run, node) {

			/* XXX need other mechanism */
			if (task->state == TASK_DEAD) {
				list_del(&task->node);
				kfree(task->stack);
				kfree(task->name);
				kfree(task);
				continue;
			}

			if (task->flags & TASK_RUN_ONCE)
				continue;

		if (max > task->attr.period)
			continue;

		max = task->attr.period;
		after = task;
	}


	if (after)
		last = ktime_add(after->wakeup, after->attr.period);


	task = list_first_entry(&tq[cpu].wake, struct task_struct, node);

	/* XXX */
	BUG_ON(task->on_cpu == KTHREAD_CPU_AFFINITY_NONE);


	if (!list_empty(&tq[cpu].run)) {

		/* reorder */

		list_for_each_entry_safe(t, tmp, &tq[cpu].run, node) {


			if (t->flags & TASK_RUN_ONCE)
				continue;

			if (t->state == TASK_DEAD)
				continue;


			first = list_first_entry(&tq[cpu].run, struct task_struct, node);
			if (ktime_before (t->wakeup, now)) {
				if (ktime_before (t->deadline - t->runtime, first->deadline)) {
					list_move(&t->node, &tq[cpu].run);
				}
			}
		}

		list_for_each_entry_safe(t, tmp, &tq[cpu].run, node) {

			if (t->flags & TASK_RUN_ONCE)
				continue;

			if (t->state != TASK_IDLE)
				continue;

			if (ktime_before (t->wakeup, now))
				continue;

			/* if the relative deadline of task-to-wake can fit in between the unused
			 * timeslice of this running task, insert after the next wakeup
			 */
			if (task->attr.deadline_rel < ktime_sub(t->deadline, t->wakeup)) {
				last = t->wakeup;
				break;
			}

			if (task->attr.wcet < ktime_sub(t->deadline, t->wakeup)) {
				last = t->deadline;
				break;
			}
		}
	}


insert:
	/* initially furthest deadline as wakeup */
	last  = ktime_add(last, 30000ULL); /* XXX minimum wakeup shift for overheads */
	task->wakeup     = ktime_add(last, task->attr.period);
	task->deadline   = ktime_add(task->wakeup, task->attr.deadline_rel);

	/* reset runtime to full */
	task->runtime = task->attr.wcet;
	task->state = TASK_IDLE;

	list_move_tail(&task->node, &tq[cpu].run);

	edf_unlock();
}




static int edf_enqueue(struct task_queue tq[], struct task_struct *task)
{
	int cpu;


	if (!task->attr.period) {
		task->flags |= TASK_RUN_ONCE;
		task->attr.period = task->attr.deadline_rel;
	} else
		task->flags &= ~TASK_RUN_ONCE;

	cpu = edf_schedulable(tq, task);
	if (cpu < 0)
		return -ENOSCHED;

	task->on_cpu = cpu;

	list_add_tail(&task->node, &tq[cpu].wake);



	return 0;
}


static ktime edf_timeslice_ns(struct task_struct *task)
{
	return (ktime) (task->runtime);
}

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
		pr_err(MSG "Cannot schedule EDF task with WCET of %llu ns, "
		           "minimum tick duration is %lld\n", attr->wcet,
			   tick_min);
		goto error;
	}

	if (ktime_delta(attr->deadline_rel, attr->wcet) < tick_min) {
		pr_err(MSG "Cannot schedule EDF task with WCET-deadline delta "
		           "of %llu ns, minimum tick duration is %lld\n",
			   ktime_delta(attr->deadline_rel, attr->wcet),
			   tick_min);
		goto error;
	}


	if (attr->period > 0) {

		if (attr->wcet >= attr->period) {
			pr_err(MSG "Cannot schedule EDF task with WCET %u >= "
			       "PERIOD %u!\n", attr->wcet, attr->period);
			goto error;
		}
		if (attr->deadline_rel >= attr->period) {
			pr_err(MSG "Cannot schedule EDF task with DEADLINE %llu >= "
			       "PERIOD %llu !\n", attr->deadline_rel, attr->period);
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
		pr_err(MSG "Cannot schedule EDF task with WCET %llu >= "
		           "DEADLINE %llu !\n", attr->wcet, attr->deadline_rel);
		goto error;
	}



	return 0;
error:

	return -EINVAL;
}


/* called after pick_next() */

ktime edf_task_ready_ns(struct task_queue *tq, int cpu, ktime now)
{
	ktime delta;
	ktime ready = (unsigned int) ~0 >> 1;

	struct task_struct *tsk;
	struct task_struct *tmp;


	list_for_each_entry_safe(tsk, tmp, &tq[cpu].run, node) {

		if (tsk->state == TASK_BUSY)
			continue;

		delta = ktime_delta(tsk->wakeup, now);

		if (delta <= (tick_get_period_min_ns() << 1)) /* XXX init once */
			continue;

		if (ready > delta)
			ready = delta;
	}


	return ready;
}


struct scheduler sched_edf = {
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
