/**
 * @file kernel/sched/edf.c
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


extern struct remove_this_declaration _kthreads;
void kthread_set_sched_policy(struct task_struct *task,
			      enum sched_policy policy);



void sched_print_edf_list_internal(ktime now)
{
//	ktime now;
	char state = 'U';
	int64_t rel_deadline;
	int64_t rel_wait;

	struct task_struct *tsk;
	struct task_struct *tmp;


//	now = ktime_get();
	ktime prev = 0;
	ktime prevd = 0;

	printk("\nt: %lld\n", ktime_to_us(now));
	printk("S\tDeadline\tWakeup\tdelta W\tdelta P\tt_rem\ttotal\tslices\tName\t\tfirstwake, firstdead, execstart\n");
	printk("----------------------------------------------\n");
	list_for_each_entry_safe(tsk, tmp, &_kthreads.run, node) {

		if (tsk->policy == SCHED_RR)
			continue;

		rel_deadline = ktime_delta(tsk->deadline, now);
		rel_wait     = ktime_delta(tsk->wakeup,   now);
		if (rel_wait < 0)
			rel_wait = 0; /* running */

		if (tsk->state == TASK_IDLE)
			state = 'I';
		if (tsk->state == TASK_RUN)
			state = 'R';

		printk("%c\t%lld\t\t%lld\t%lld\t%lld\t%lld\t%lld\t%d\t%s %lld | %lld %lld %lld %lld\n",
		       state, ktime_to_us(tsk->deadline), ktime_to_us(tsk->wakeup),
		       ktime_to_us(rel_wait), ktime_to_us(rel_deadline), ktime_to_us(tsk->runtime), ktime_to_us(tsk->total),
		       tsk->slices, tsk->name, ktime_us_delta(prev, tsk->wakeup), ktime_us_delta(prevd, tsk->deadline),
		       ktime_to_us(tsk->first_wake),
		       ktime_to_us(tsk->first_dead),
		       ktime_to_us(tsk->exec_start));
		prev = tsk->wakeup;
		prevd = tsk->deadline;
	}
}
ktime total;
ktime times;
void sched_print_edf_list(void)
{
	printk("avg: %lld\n", ktime_to_us(total/times));
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
		printk("%s violated, %lld %lld, %lld %lld\n", tsk->name,
		       tsk->runtime, ktime_us_delta(tsk->deadline, now),
		       tsk->deadline, now);
		sched_print_edf_list_internal(now);
		BUG();
		return false;
	}

	to_deadline = ktime_delta(tsk->deadline, now);

	if (to_deadline <= 0)
		return false;



	if (tsk->wcet * to_deadline < tsk->period * tsk->runtime)
		return true;





	return false;
}


static inline void schedule_edf_reinit_task(struct task_struct *tsk, ktime now)
{
	tsk->state = TASK_IDLE;

	tsk->wakeup = ktime_add(tsk->wakeup, tsk->period);
#if 0
	if (ktime_after(now, tsk->wakeup))
		printk("%s delta %lld\n",tsk->name, ktime_us_delta(tsk->wakeup, now));

	BUG_ON(ktime_after(now, tsk->wakeup)); /* deadline missed earlier? */
#endif
	tsk->deadline = ktime_add(tsk->wakeup, tsk->deadline_rel);

	tsk->runtime = tsk->wcet;
}


#define SOME_DEFAULT_TICK_PERIOD_FOR_SCHED_MODE 10000000LL
/* stupidly sort EDFs */
/*static*/ int64_t schedule_edf(ktime now)
{
//	ktime now;

	int64_t delta;

	int64_t slot = SOME_DEFAULT_TICK_PERIOD_FOR_SCHED_MODE;

	struct task_struct *tsk;
	struct task_struct *tmp;
	ktime wake;



	list_for_each_entry_safe(tsk, tmp, &_kthreads.run, node) {

		if (tsk->policy != SCHED_EDF)
			continue;

		/* time to wake up yet? */
		delta = ktime_delta(tsk->wakeup, now);
		if (delta >= 0) {
			/* nope, just update minimum runtime for this slot */
			if (delta < slot)
				slot = delta;

			continue;
		}

		/* if it's already running, see if there is time remaining */
		if (tsk->state == TASK_RUN) {
			if (!schedule_edf_can_execute(tsk, now)) {
				schedule_edf_reinit_task(tsk, now);
				/* nope, update minimum runtime for this slot */
				delta = ktime_delta(tsk->wakeup, now);
				if (delta < slot)
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
#if 1
	total = ktime_add(total, ktime_delta(ktime_get(), now));
	times++;
#endif
#if 0
	printk("%3.d %3.lld\n", cnt, ktime_to_us(total / times) );
#endif
	BUG_ON(slot < 0);

	return slot;
}


/**
 *
 * we allow online task admission, so we need to be able to determine
 * schedulability on the fly:
 *
 * EDF schedulability
 *
 *
 * # comp time
 * | deadline (== unused slot)
 * _ unused slot
 * > wakeup (== deadline if D == P)
 * o free slots (deadline - wcet)
 *
 * simplest case: one long period task, one or more short period tasks
 *
 *  W                    D                             W
 *  >oooooooooo##########|_____________________________> (P=50, D=20, R=10) (T1)
 *  >o#|_>                                               (P= 4, D= 2, R= 1) (T2)
 *  >o#>                                                 (P= 2, D= 2, R= 1) (T3)
 *  >#>                                                  (P= 1, D= 1, R= 1) (T4)
 *  >ooooooo#####|_______>                               (P=20, D=12, R= 5) (T5)
 *  >oooooooooooooooooooooooooo####|__>			 (P=33, D=30, R= 4) (T6)
 *  >oooooooooooooooooooooooooooooooooooooooo######|___> (P=50, D=46, R= 6) (T7)
 *
 * If we map the short period task into the long period tasks "free" slots,
 * we see that tasks with periods shorter than the deadline of the task
 * of the longest period can only be scheduled, if their utilisation
 * or "time density" R / D is smaller that the utilisation of the longest
 * period task
 *
 *
 *  easily schedulable:
 *   ____________________________________________________________________________________________________
 *   .... .   .. .........    .   .  .    .   .   .   .....   . ... .......   .   .   .   .   .   .   .
 *  >o###oooooo#oo####o##|_____________________________###oooooo##oo##o###o|_____________________________
 *  >#o|_o#|_o#|_#o|_o#|_#o|_o#|_o#|_#o|_o#|_o#|_o#|_o#|_o#|_o#|_o#|_o#|_o#|_o#|_o#|_o#|_o#|_o#|_o#|_o#|_
 *
 *
 *                            R/D                     R/P
 *  T1: (P=50, D=20, R=10)   10/20 = 1/2        10/50 = 20/100
 *  T2: (P= 4, D= 2, R= 1)    1/2  = 1/2 100%    1/4  = 25/100 45% (== number of used slots)
 *
 *
 *  correct analysis sum(R_i/P_i)
 *
 *  T1 (D-R) / D = 1/2
 *  T1 (D-R) / P = 1/5
 *
 *  T2 (D-R) / P = 1/4	 T1DRD > T2DRP	-> R/P (correct)
 *
 *
 *
 *  just schedulable:
 *   ____________________________________________________________________________________________________
 *   .................... . . . . . . . . . . . . . . ..................... . . . . . . . . . . . . . . .
 *  >#o#o#o#o#o#o#o#o#o#o|_____________________________#o#o#o#o#o#o#o#o#o#o|_____________________________
 *  >o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#o#
 *
 *                            R/D                     R/P
 *  T1: (P=50, D=20, R=10)   10/20 = 1/2        10/50 = 2/10
 *  T3: (P= 2, D= 2, R= 1)    1/2  = 1/2 100%    1/2  = 5/10  70% (== number of used slots)
 *
 *  -> this must be 100%, impossible to fit more slots into relative deadline of
 *     long task
 *
 *  correct analysis sum(R_i/D_i)
 *
 *  T1 (D-R) / D = 1/2
 *  T1 (D-R) / P = 1/5
 *
 *  T3 (D-R) / P = 1/2		T1DRD <= T3DRP	-> R/D (correct)
 *
 *  not schedulable:
 *   ____________________________________________________________________________________________________
 *   ..........::::::::::........................................::::::::::..............................
 *  >oooooooooo##########|_____________________________oooooooooo##########|_____________________________
 *  >####################################################################################################
 *
 *                            R/D                     R/P
 *  T1: (P=50, D=20, R=10)   10/20 = 1/2        10/50 = 1/5
 *  T4: (P= 1, D= 1, R= 1)    1/1  = 1/1 150%    1/1  = 1/1 120%
 *
 *  both correct, but R/P "more correct" -> actual slot usage
 *
 *  T1 (D-R) / D = 1/2
 *  T1 (D-R) / P = 1/5
 *
 *  T4 (D-R) / P = 0		T1DRD > T4DRD	-> R/P	(correct)
 *
 *
 *  schedulable:
 *
 *   ____________________________________________________________________________________________________
 *   .................................................................................................xxx
 *  >o###oooooo#oo###o###|_____________________________##o###o#ooooooo###o#|_____________________________
 *  >#o|_#o|_o#|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_
 *  >ooooo####oo#|_______o###oooooo##|_______o#ooo###o#oo|_______o###oooooo##|_______o###oooooo##|_______
 *  >ooooooooooooooooooooooooo###o#|__ooooooooo##ooppoooooooooo##ooo|__ooooooooooooooooooo###o#oooooo|__x
 *  >ooooooooooooooooooooooooooooooooo###o###oooooo|___ooooooooooooooooooooooo###o###ooooooooooooo###|___
 *
 *                            R/D            R/P
 *  T1: (P=50, D=20, R=10)   10/20   50%    10/50  20%
 *  T2: (P= 4, D= 2, R= 1)    1/2   100%     1/4   45%
 *  T5: (P=20, D=12, R= 5)    5/12  142%     5/20  70%
 *  T6: (P=33, D=30, R= 4)    4/30  155%     4/33  82%
 *  T7: (P=50, D=46, R= 6)    6/46  168%     6/50  94%
 *
 *
 *
 *  sum(R_i/P_i) correct, sum(R_i/D_i) absolutely incorrect!
 *
 *  thread(p_max):
 *  T1 (D-R) / D = 1/2
 *  T1 (D-R) / P = 1/5
 *  ------------------
 *  T2 (D-R) / P = 1/4			T1DRD > T2DRP -> R/P (correct)
 *  T5 (D-R) / P = 7/20			T1DRD > T5DRP -> R/P (correct)
 *  T6 (D-R) / P = 26/33		T1RD <= T6DRP -> R/D (correct? looks ok)
 *  T7 (D-R) / P = 40/50		T1RD <= T6DRP -> R/D (correct? looks ok)
 *
 *  usage: 96.4%
 *
 *
 *
 *
 *
 * try 1:
 *
 *  T1: (P=50, D=20, R=10) (20%) T1DRP = 0.2 (0.95)
 *  TX: (P=10, D= 8, R=6) -> (D-R)/P = 0.2  T1DRD > T2DRP -> R/P (incorrect, should be R/D at least) (0.75)
 *  ................::..
 * >##oooooo####oooo####|_____________________________
 * >oo######|_oo######|_
 *
 * 22/20 slots used = 110%
 *
 * free T1 slots before deadline:	D-R = 10
 *
 * TX runtime slots cannot be larger than that!
 *
 * TX runtime slots for T1 deadline periods:
 *
 *
 *	(D_x - R_x) / D_x * D_1 = 12.5  < 10 -> not schedulable
 *
 *	sum((D_i - R_i) / D_i) * D_1 < 10 -> schedulable?
 *
 *
 *
 *	i != D_1 && P_i < D_1
 *		sum((D_1 / P_i) * R_i) < (D_1 - R_1) ?
 *
 *	i != D1 && P_i >
 *
 *	(T1: 4 < 10 ok)
 *
 *	T2: 5
 *	T5: 5
 *	T6  2.42
 *
 *
 *
 *
 * new test:
 *
 * if (P_x < D_1) :
 *	 if ((R_x - D_x) > 0) // otherwise we are at 100% slot usage within deadline
 *		(D_x - R_x) / D_x * (D_1 - R_1) = 12.5 > (D_1 - R_1) -> not schedulable
 *	 else ?
 *		R_x / P_x * D_1 < (D_1 - R_1)
 *
 * (schedulable):
 *   ____________________________________________________________________________________________________
 *   .................................................................................................xxx
 *  >o###oooooo#oo###o###|_____________________________##o###o#ooooooo###o#|_____________________________
 *  >#o|_#o|_o#|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_
 *  >ooooo####oo#|_______o###oooooo##|_______o#ooo###o#oo|_______o###oooooo##|_______o###oooooo##|_______
 *  >ooooooooooooooooooooooooo###o#|__ooooooooo##ooppoooooooooo##ooo|__ooooooooooooooooooo###o#oooooo|__x
 *  >ooooooooooooooooooooooooooooooooo###o###oooooo|___ooooooooooooooooooooooo###o###oooooooooooooooo|___
 *
 *
 *  T1: (P=50, D=20, R=10) -> max P -> D_1 = 20, D_1 - R_1 = 10 = F_1
 *
 *  T2: (P= 4, D= 2, R= 1)  F2 = 1   F2D = 1/2
 *  T5: (P=20, D=12, R= 5)  F5 = 7   F5D = 7/12
 *  T6: (P=33, D=30, R= 4)  F6 = 26  F6D = 26/30
 *  T7: (P=50, D=46, R= 6)  F7 = 40  F7D = 40/46
 *
 *
 * Utilisation:	U1 = D_1 - R_1 = 10;	U2 = P_1 - D_1 = 30
 *
 * check T2:
 *	f2 > 0 ->  f2d * F1 = 5 <= U1 -> schedulable
 *		   f2d * U2 = 10
 *		-> U1 = 5, U2 = 20
 *
 * check t5:
 *	f5 > 0 -> int(f5d * F1) = 5;	5 <= U1 -> schedulable
 *		   f5d * U2 = int(11) ->
 *		   U1 = 0, U2 = 9
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
 *
 *	if (DEADLINE >
 *   ____________________________________________________________________________________________________
 *   .........................   . ............  ..............  .........................   . ...   .
 *  >o###oooooo#oo###o###|_____________________________##o###o#ooooooo###o#|_____________________________
 *  >#o|_#o|_o#|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_#o|_
 *  >ooooo####oo#|_______o###oooooo##|_______o#ooo###o#oo|_______o###oooooo##|_______o###oooooo##|_______
 *  >ooooooooooooooooooooooooooooooooo###o###oooooo|___ooooooooooooooooooooooo###o###oooooooooooooooo|___
 *
 *
 *
 */

static ktime hyperperiod(void)
{
	ktime lcm = 0;

	ktime a,b;

	struct task_struct *t0;
	struct task_struct *tsk;
	struct task_struct *tmp;


	if (list_empty(&_kthreads.new))
	    return 0;

	t0 = list_entry(_kthreads.new.next, struct task_struct, node);

	lcm = t0->period;

	list_for_each_entry_safe(tsk, tmp, &_kthreads.new, node) {

		if (tsk == t0)
			continue;

		a = lcm;
		b = tsk->period;

		/* already a multiple? */
		if (a % b == 0)
			continue;

		while (a != b) {
			if (a > b)
				a -= b;
			else
				b -= a;
		}

		lcm = lcm * (tsk->period / a);
	}

	return lcm;
}

#define MIN(A,B)	((A) < (B) ? (A) : (B))

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
	//task->runtime  = ktime_sub(task->wcet, 7000LL);
	task->runtime  = task->wcet;
	task->deadline_rel = us_to_ktime(deadline_rel_us);
	arch_local_irq_enable();

	printk("\nvvvv EDF analysis vvvv\n\n");
	{
		ktime p = hyperperiod();
		ktime h ;
		ktime max = 0;

		ktime uh, ut, f1;

		struct task_struct *t0 = NULL;
		struct task_struct *tsk = NULL;
		struct task_struct *tmp;

		if (p < 0)
			printk("appears to be empty\n");

		list_for_each_entry_safe(tsk, tmp, &_kthreads.new, node) {

			if (tsk->period > max) {
				t0 = tsk;
				max = tsk->period;
			}
		}

		BUG_ON(!t0);

		BUG_ON(p < t0->period);
		h = p / t0->period;

		printk("Period factor %lld, duration %lld actual min period: %lld\n", h, ktime_to_ms(p), ktime_to_ms(t0->period));



		uh = h * (t0->deadline_rel - t0->wcet);
		ut = h * (t0->period - t0->deadline_rel);
		f1 = ut/h;


		printk("max UH: %lld, UT: %lld\n", ktime_to_ms(uh), ktime_to_ms(ut));


		list_for_each_entry_safe(tsk, tmp, &_kthreads.new, node) {
			ktime sh, st;

			if (tsk == t0)
				continue;


			if (tsk->deadline_rel < t0->deadline_rel) {

				/* slots before deadline of  T0 */
				sh = h * tsk->wcet * t0->deadline_rel / tsk->period;

				if (sh > uh) {
					printk("NOT SCHEDULABLE in head: %s\n", tsk->name);
					BUG();
				}
				uh = uh - sh;

			}

			/* slots after deadline of T0 */
			st = h * tsk->wcet * f1 / tsk->period;
			printk("%s tail usage: %lld\n", tsk->name, ktime_to_ms(st));
			if (st > ut) {
				printk("NOT SCHEDULABLE in tail: %s\n", tsk->name);
				BUG();
			}
			ut = ut - st;


			printk("UH: %lld, UT: %lld\n", ktime_to_ms(uh), ktime_to_ms(ut));


		}


	}




	{
		double u = 0.0;	/* utilisation */

		struct task_struct *tsk;
		struct task_struct *tmp;

		static int64_t dmin = 0x7fffffffffffffLL;


		if (dmin > task->deadline_rel)
			dmin = task->deadline_rel;


		u += (double) (int32_t) task->wcet / (double) (int32_t) task->period;


		list_for_each_entry_safe(tsk, tmp, &_kthreads.new, node) {

			if (tsk->policy != SCHED_EDF)
				continue;

			u += (double) (int32_t) tsk->wcet / (double) (int32_t) tsk->period;
		}

		if (u > 1.0) {
			printk("I am NOT schedul-ableh: %g ", u);
			kthread_set_sched_policy(task, SCHED_RR);
			printk("changed task mode to RR\n", u);
		} else {
			printk("Utilisation %g\n", u);
			kthread_set_sched_policy(task, SCHED_EDF);
		}
	}

	printk("\n^^^^ EDF analysis ^^^^\n");


//	arch_local_irq_enable();


	printk("\n");

}

