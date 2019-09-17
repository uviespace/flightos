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


	static int once;
	if (!once) {

//	tick_set_mode(TICK_MODE_PERIODIC);
	tick_set_next_ns(1e9);	/* XXX default to 1s ticks initially */
	once = 1;
	return;
	}


	arch_local_irq_disable();
	/* kthread_lock(); */

	/* get the current task for this CPU */
	current = current_set[0]->task;

	/** XXX need timeslice_update callback for schedulers */
	/* update remaining runtime of current thread */
	current->runtime = ktime_sub(current->exec_start, ktime_get());


retry:
	/* XXX: for now, try to wake up any threads not running
	 * this is a waste of cycles and instruction space; should be
	 * done in the scheduler's code (somewhere) */
	list_for_each_entry(sched, &kernel_schedulers, node) {
		sched->wake_next_task(&sched->tq);
	}


	/* XXX need sorted list: highest->lowest scheduler priority, e.g.:
	 * EDF -> RMS -> FIFO -> RR
	 * TODO: scheduler priority value
	 */

	list_for_each_entry(sched, &kernel_schedulers, node) {



		/* if one of the schedulers have a task which needs to run now,
		 * next is non-NULL
		 */
		next = sched->pick_next_task(&sched->tq);
#if 0
		if (next)
			printk("next %s %llu %llu\n", next->name, next->first_wake, ktime_get());
		else
			printk("NULL\n");
#endif

		/* check if we need to limit the next tasks timeslice;
		 * since our scheduler list is sorted by scheduler priority,
		 * only update the value if wake_next is not set;
		 *
		 * because our schedulers are sorted, this means that if next
		 * is set, the highest priority scheduler will both tell us
		 * whether it has another task pending soon. If next is not set,
		 * a lower-priority scheduler may set the next thread to run,
		 * but we will take the smallest timeout from high to low
		 * priority schedulers, so we enter this function again when
		 * the timeslice of the next thread is over and we can determine
		 * what needs to be run in the following scheduling cycle. This
		 * way, we will distribute CPU time even to the lowest priority
		 * scheduler, if available, but guarantee, that the highest
		 * priority threads are always ranked and executed on time
		 *
		 * we assume that the timeslice is reasonable; if not fix it in
		 * the corresponding scheduler
		 */

		if (!wake_ns)
			wake_ns = sched->task_ready_ns(&sched->tq);

		/* we found something to execute, off we go */
		if (next)
			break;
	}


	if (!next) {
		/* there is absolutely nothing nothing to do, check again later */
		if (wake_ns)
			tick_set_next_ns(wake_ns);
		else
			tick_set_next_ns(1e9);	/* XXX pause for a second, there are no threads in any scheduler */

		goto exit;
	}

	/* see if the remaining runtime in a thread is smaller than the wakeup
	 * timeout. In this case, we will restrict ourselves to the remaining
	 * runtime. This is particularly needeed for strictly periodic
	 * schedulers, e.g. EDF
	 */

	slot_ns = next->sched->timeslice_ns(next);

	if (wake_ns < slot_ns)
		slot_ns  = wake_ns;

	/* statistics */
	next->exec_start = ktime_get();
//	printk("at %llu\n", next->exec_start);

//		if (next)
//			printk("real next %s %llu %llu\n", next->name, next->exec_start, slot_ns);
	/* kthread_unlock(); */

//	printk("wake %llu\n", ktime_to_us(slot_ns));

	/* subtract readout overhead */
	tick_set_next_ns(ktime_sub(slot_ns, 2000LL));
#if 1
	if (slot_ns < 20000UL) {
		goto retry;
		printk("wake %llu slot %llu %s\n", wake_ns, slot_ns, next->name);
		BUG();
	}
#endif
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
