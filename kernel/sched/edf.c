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


#if 1

void sched_print_edf_list_internal(struct task_queue *tq, ktime now)
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

	printk("\nktime: %lld\n", ktime_to_us(now));
	printk("S\tDeadline\tWakeup\tdelta W\tdelta P\tt_rem\ttotal\tslices\tName\t|\tdeltaw\tdeltad\twake0\tdead0\texecstart\n");
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

		printk("%c\t%lld\t\t%lld\t%lld\t%lld\t%lld\t%lld\t%d\t%s\t|\t%lld\t%lld\t%lld\t%lld\t%lld\n",
		       state, ktime_to_us(tsk->deadline), ktime_to_us(tsk->wakeup),
		       ktime_to_us(rel_wait), ktime_to_us(rel_deadline), ktime_to_us(tsk->runtime), ktime_to_us(tsk->total),
		       tsk->slices, tsk->name,
		       ktime_us_delta(prev, tsk->wakeup),
		       ktime_us_delta(prevd, tsk->deadline),
		       ktime_to_us(tsk->first_wake),
		       ktime_to_us(tsk->first_dead),
		       ktime_to_us(tsk->exec_start));
		prev = tsk->wakeup;
		prevd = tsk->deadline;
	}

	printk("\n\n");
}
ktime total;
ktime times;
void sched_print_edf_list(void)
{
	printk("avg: %lld\n", ktime_to_us(total/times));
}
#endif
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
}
#if 0

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

		if (tsk->attr.policy != SCHED_EDF)
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

		if (tsk->attr.policy != SCHED_EDF)
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

#endif

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

static ktime edf_hyperperiod(struct task_queue *tq)
{
	ktime lcm = 0;

	ktime a,b;

	struct task_struct *t0;
	struct task_struct *tsk;
	struct task_struct *tmp;


	if (list_empty(&tq->new))
	    return 0;

	t0 = list_entry(tq->new.next, struct task_struct, node);

	lcm = t0->attr.period;

	list_for_each_entry_safe(tsk, tmp, &tq->new, node) {

		if (tsk == t0)
			continue;

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

	/* argh, need to consider everything */
	list_for_each_entry_safe(tsk, tmp, &tq->run, node) {

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
	list_for_each_entry_safe(tsk, tmp, &tq->wake, node) {

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


/**** NEW EDF ****/

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
 */

static int edf_schedulable(struct task_queue *tq, const struct task_struct *task)
{


	ktime p = edf_hyperperiod(tq);
	ktime h ;
	ktime max = 0;

	ktime uh, ut, f1;

		ktime sh = 0, st = 0;
	struct task_struct *t0 = NULL;
	struct task_struct *tsk = NULL;
	struct task_struct *tmp;

	double u = 0.0;	/* utilisation */


	static int64_t dmin = 0x7fffffffffffffLL;

//	printk("\nvvvv EDF analysis vvvv (%lld us) \n\n", ktime_to_us(p));

	/* list_empty(....) */

	if (p <= 0)
		printk("appears to be empty\n");

	list_for_each_entry_safe(tsk, tmp, &tq->new, node) {
		if (tsk->attr.period > max) {
			t0 = tsk;
			max = tsk->attr.period;
		}
	}

	list_for_each_entry_safe(tsk, tmp, &tq->wake, node) {
		if (tsk->attr.period > max) {
			t0 = tsk;
			max = tsk->attr.period;
		}
	}

	list_for_each_entry_safe(tsk, tmp, &tq->run, node) {
		if (tsk->attr.period > max) {
			t0 = tsk;
			max = tsk->attr.period;
		}
	}



	BUG_ON(!t0);

	BUG_ON(p < t0->attr.period);
	h = p / t0->attr.period;

	printk("Period factor %lld, duration %lld actual min period: %lld\n", h, ktime_to_us(p), ktime_to_us(t0->attr.period));



	uh = h * (t0->attr.deadline_rel - t0->attr.wcet);
	ut = h * (t0->attr.period - t0->attr.deadline_rel);
	f1 = ut/h;



	printk("max UH: %lld, UT: %lld\n", ktime_to_us(uh), ktime_to_us(ut));

	/* subtract longest period thread from head, its slices must always
	 * be used before the deadline
	 */
	sh = h * t0->attr.wcet * t0->attr.deadline_rel / t0->attr.period;
	uh = uh - sh;
	printk("%s UH: %lld, UT: %lld\n", t0->name, ktime_to_us(uh), ktime_to_us(ut));
	printk("%s SH: %lld, ST: %lld\n", t0->name, ktime_to_us(sh), ktime_to_us(st));


	/* add all in wakeup */
	struct task_struct *tmp2;
	if (!list_empty(&tq->wake)) {
	list_for_each_entry_safe(tsk, tmp, &tq->wake, node) {

		if (tsk == t0)
			continue;

		if (tsk->attr.policy != SCHED_EDF)
			continue;

		u += (double) (int32_t) tsk->attr.wcet / (double) (int32_t) tsk->attr.period;


		if (tsk->attr.deadline_rel < t0->attr.deadline_rel) {

			/* slots before deadline of  T0 */
			sh = h * tsk->attr.wcet * t0->attr.deadline_rel / tsk->attr.period;

			if (sh > uh) {
				printk("NOT SCHEDULABLE in head: %s\n", tsk->name);
				BUG();
			}
			uh = uh - sh;

		}

		/* slots after deadline of T0 */
		st = h * tsk->attr.wcet * f1 / tsk->attr.period;
//		printk("%s tail usage: %lld\n", tsk->name, ktime_to_ms(st));
		if (st > ut) {
			printk("NOT SCHEDULABLE in tail: %s\n", tsk->name);
			BUG();
		}
		ut = ut - st;


		printk("%s UH: %lld, UT: %lld\n", tsk->name, ktime_to_us(uh), ktime_to_us(ut));
		printk("%s SH: %lld, ST: %lld\n", tsk->name, ktime_to_us(sh), ktime_to_us(st));
	}
	}

	/* add all running */
	if (!list_empty(&tq->run)) {
	list_for_each_entry_safe(tsk, tmp, &tq->run, node) {

		if (tsk == t0)
			continue;

		if (tsk->attr.policy != SCHED_EDF)
			continue;

		u += (double) (int32_t) tsk->attr.wcet / (double) (int32_t) tsk->attr.period;


		if (tsk->attr.deadline_rel < t0->attr.deadline_rel) {

			/* slots before deadline of  T0 */
			sh = h * tsk->attr.wcet * t0->attr.deadline_rel / tsk->attr.period;

			if (sh > uh) {
				printk("NOT SCHEDULABLE in head: %s\n", tsk->name);
				BUG();
			}
			uh = uh - sh;

		}

		/* slots after deadline of T0 */
		st = h * tsk->attr.wcet * f1 / tsk->attr.period;
//		printk("%s tail usage: %lld\n", tsk->name, ktime_to_ms(st));
		if (st > ut) {
			printk("NOT SCHEDULABLE in tail: %s\n", tsk->name);
			BUG();
		}
		ut = ut - st;


		printk("UH: %lld, UT: %lld\n", ktime_to_us(uh), ktime_to_us(ut));
		printk("SH: %lld, ST: %lld\n", ktime_to_us(sh), ktime_to_us(st));
	}
	}

#if 0
	//list_for_each_entry_safe(tsk, tmp, &tq->new, node)
	{
		tsk = t0;
		ktime sh, st;

	//	if (tsk == t0)
	//		continue;


		if (tsk->attr.deadline_rel < t0->attr.deadline_rel) {

			/* slots before deadline of  T0 */
			sh = h * tsk->attr.wcet * t0->attr.deadline_rel / tsk->attr.period;

			if (sh > uh) {
				printk("NOT SCHEDULABLE in head: %s\n", tsk->name);
				BUG();
			}
			uh = uh - sh;

		}

		/* slots after deadline of T0 */
		st = h * tsk->attr.wcet * f1 / tsk->attr.period;
//		printk("%s tail usage: %lld\n", tsk->name, ktime_to_ms(st));
		if (st > ut) {
			printk("NOT SCHEDULABLE in tail: %s\n", tsk->name);
			BUG();
		}
		ut = ut - st;


		printk("UH: %lld, UT: %lld\n", ktime_to_us(uh), ktime_to_us(ut));
		printk("SH: %lld, ST: %lld\n", ktime_to_us(sh), ktime_to_us(st));






		if (dmin > task->attr.deadline_rel)
			dmin = task->attr.deadline_rel;


		u += (double) (int32_t) task->attr.wcet / (double) (int32_t) task->attr.period;
#endif




		if (u > 1.0) {
			printk("I am NOT schedul-ableh: %f ", u);
			return -EINVAL;
			printk("changed task mode to RR\n", u);
		} else {
			printk("Utilisation: %g\n", u);
			return 0;
		}
//	}


	u = (double) (int32_t) task->attr.wcet / (double) (int32_t) task->attr.period;

//	printk("was the first task, utilisation: %g\n", u);

	return 0;
}

/** XXX **/
static int64_t slot;




static struct task_struct *edf_pick_next(struct task_queue *tq)
{
#define SOME_DEFAULT_TICK_PERIOD_FOR_SCHED_MODE 111111LL
	int64_t delta;


	struct task_struct *go = NULL;
	struct task_struct *tsk;
	struct task_struct *tmp;
	struct task_struct *first;
	ktime now = ktime_get();
	slot = 1000000000000; //SOME_DEFAULT_TICK_PERIOD_FOR_SCHED_MODE;


	list_for_each_entry_safe(tsk, tmp, &tq->run, node) {

//		printk("checking %s\n", tsk->name);
		/* time to wake up yet? */
		delta = ktime_delta(tsk->wakeup, now);

		if (delta > 0) {

			/* nope, just update minimum runtime for this slot */

			if (delta < slot) {
				slot = delta;
//				printk("d %lld now: %lld \n", ktime_to_us(delta), now);
			}
		//	printk("delta %llu %llu\n", delta, tsk->wakeup);

			continue;
		}
		if (delta < 0) {

		//	printk("\n%lld %d\n", ktime_to_us(delta), tsk->state);
		//	printk("%s: %lld (%lld) deadline: %lld now: %lld state %d\n", tsk->name, ktime_to_ms(delta), tsk->wakeup, tsk->deadline, now, tsk->state);
		}

		/* if it's already running, see if there is time remaining */
		if (tsk->state == TASK_RUN) {

			if (!schedule_edf_can_execute(tsk, now)) {
				schedule_edf_reinit_task(tsk, now);
			//	printk("reinit %s\n", tsk->name);
				/* nope, update minimum runtime for this slot */
				delta = ktime_delta(tsk->wakeup, now);

				/* if wakeup must happen earlier than the next
				 * scheduling event, adjust the slot timeout
				 */
				if (delta < slot) {
					slot = delta;
				//	printk("slot %lld\n", ktime_to_us(delta));
				}

				if (delta < 0)
					printk("delta %lld %lld\n", ktime_to_us(delta), ktime_to_us(tick_get_period_min_ns()));
				BUG_ON(delta < 0);

			//	continue;
			}


		//	if (tsk->runtime < tsk->attr.wcet)
		//		printk("VVV %s %lld %lld\n", tsk->name, tsk->runtime, tsk->attr.wcet);

			/* if our deadline is earlier than the deadline at the
			 * head of the list, move us to top */

			first = list_first_entry(&tq->run, struct task_struct, node);

			if (ktime_before (tsk->deadline, first->deadline)) {
				tsk->state = TASK_RUN;
			//	go = tsk;
				list_move(&tsk->node, &tq->run);
			//	printk("1 to top! %s\n", tsk->name);
			}
		//		printk("1 nope %s\n", tsk->name);

			continue;
		}

		/* time to wake up */
		if (tsk->state == TASK_IDLE) {
			tsk->state = TASK_RUN;
			/* move to top */

		//	printk("%s now in state RUN\n", tsk->name);


			/* if our deadline is earlier than the deadline at the
			 * head of the list, move us to top */

			first = list_first_entry(&tq->run, struct task_struct, node);

			if (ktime_before (tsk->deadline, first->deadline)) {
			//	go = tsk;
				list_move(&tsk->node, &tq->run);
		//		printk("%s has earlier deadline, moved to top\n", tsk->name);
			}
			//	printk("2 nope %s\n", tsk->name);

			continue;
		}

	}

	first = list_first_entry(&tq->run, struct task_struct, node);
	delta = ktime_delta(first->wakeup, now);
//	if (delta <= 0)
       if (first->state == TASK_RUN) {
		go = first;
		slot = first->runtime;
       }

#if 0
	list_for_each_entry_safe(tsk, tmp, &tq->run, node) {
		printk("%c %s %lld %lld\n",
		       (tsk->state == TASK_RUN) ? 'R' : 'I',
			tsk->name,
			ktime_to_ms(ktime_delta(tsk->wakeup, now)),
			ktime_to_ms(ktime_delta(tsk->deadline, now))
		       );
	}
#endif
//	if (!go)
//		printk("NULL\n");
//	printk("in %llu\n", ktime_to_ms(slot));
#if 0
	/** XXX **/
	tsk = list_entry(tq->run.next, struct task_struct, node);

	if (tsk->state == TASK_RUN)
		return tsk;
	else
#endif
	return go;
}


static void edf_wake_next(struct task_queue *tq)
{
	struct task_struct *tsk;
	struct task_struct *tmp;
	struct task_struct *task;

	ktime last=0;
	ktime per = 50000;

	if (list_empty(&tq->wake))
		return;

	task = list_entry(tq->wake.next, struct task_struct, node);


	last = ktime_get();

	list_for_each_entry_safe(tsk, tmp, &tq->run, node) {
		if (tsk->deadline > last)
			last = tsk->deadline;

		if (tsk->attr.period > per)
			per = tsk->attr.period;
	}

//	if (next->attr.policy == SCHED_EDF)
//		return;
//
	/* initially furthest deadline as wakeup */
	task->wakeup = ktime_add(last, task->attr.period);
	/* add overhead */
//	task->wakeup = ktime_add(task->wakeup, 50000UL);
#if 0
	task->wakeup = ktime_add(task->wakeup, per);
#endif

	task->deadline = ktime_add(task->wakeup, task->attr.deadline_rel);
	task->first_wake = task->wakeup;
	task->first_dead = task->deadline;
	task->state = TASK_IDLE;

//	printk("---- %s %llu\n", task->name, task->first_wake);

	list_move_tail(&task->node, &tq->run);
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


ktime edf_task_ready_ns(struct task_queue *tq)
{
	int64_t delta;


	struct task_struct *tsk;
	struct task_struct *tmp;
	ktime now = ktime_get();
	ktime wake = 123456789123LL;



//	wake = ktime_add(now, slot);
	list_for_each_entry_safe(tsk, tmp, &tq->run, node) {
#if 0
		if (tsk->state == TASK_IDLE)
			continue;
#endif

		delta = ktime_delta(now, tsk->wakeup);
		if (delta <= 0)
		    continue;

		if (wake > delta)
			wake = delta;
#if 0
		/* all currently runnable task are at the top of the list */
		if (tsk->state != TASK_RUN)
			break;
#endif
#if 0
		if (ktime_before(wake, tsk->wakeup))
			continue;

		delta = ktime_delta(wake, tsk->wakeup);

		if (delta < 0) {
			delta = ktime_delta(now, tsk->wakeup);
			printk("\n [%lld] %s wakeup violated by %lld us\n", ktime_to_ms(now), tsk->name, ktime_to_us(delta));
		}


		if (delta < slot) {
			if (delta)
				slot = delta;
			else
				delta = tsk->runtime;
		//	wake = ktime_add(now, slot); /* update next wakeup */
			/* move to top */
		//	list_move(&tsk->node, &tq->run);
			BUG_ON(slot <= 0);
		}
#endif
	}

	/* subtract call overhead */
//	slot = wake;
 	//slot = ktime_sub(slot, 10000ULL);
 	//slot = ktime_sub(slot, 2000ULL);
	//
	if (slot > wake) {
		printk("\nvvvvvvvvvvvvvvv\n");
		printk("Slice adjusted from %lld to %lld (%lld)\n", ktime_to_us(slot), ktime_to_us(wake), ktime_to_us(wake - slot));
		printk("\n^^^^^^^^^^^^^^^\n");
		slot = wake;

	}
	BUG_ON(slot <= 0);

	return slot;
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
	/* XXX */
	INIT_LIST_HEAD(&sched_edf.tq.new);
	INIT_LIST_HEAD(&sched_edf.tq.run);
	INIT_LIST_HEAD(&sched_edf.tq.wake);
	INIT_LIST_HEAD(&sched_edf.tq.dead);

	sched_register(&sched_edf);

	return 0;
}
postcore_initcall(sched_edf_init);
