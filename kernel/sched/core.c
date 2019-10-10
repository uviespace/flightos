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

#include <asm/leon.h>


#define MSG "SCHEDULER: "

static LIST_HEAD(kernel_schedulers);


/* XXX: per-cpu */
extern struct thread_info *current_set[];

ktime sched_last_time;
uint32_t sched_ev;

 void kthread_lock(void);
 void kthread_unlock(void);
void schedule(void)
{
	struct scheduler *sched;

	struct task_struct *next = NULL;
	struct task_struct *prev = NULL;

	struct task_struct *current;
	int64_t slot_ns = 1000000LL;
	int64_t wake_ns = 1000000000;

	ktime rt;
	ktime now;


	static int once[4];
	if (!once[leon3_cpuid()]) {

//	tick_set_mode(TICK_MODE_PERIODIC);
	tick_set_next_ns(1e9);	/* XXX default to 1s ticks initially */
	once[leon3_cpuid()] = 1;
	return;
	}



	arch_local_irq_disable();
//	if (leon3_cpuid() != 0)
//	printk("cpu %d\n", leon3_cpuid());
kthread_lock();

	/* get the current task for this CPU */
	/* XXX leon3_cpuid() should be smp_cpu_id() arch call*/
	current = current_set[leon3_cpuid()]->task;


//	if (leon3_cpuid() != 0)
//	if (current)
//		printk("current %s\n", current->name);

	now = ktime_get();

	rt = ktime_sub(now, current->exec_start);

	/** XXX need timeslice_update callback for schedulers */
	/* update remaining runtime of current thread */

	current->runtime = ktime_sub(current->runtime, rt);
	current->total = ktime_add(current->total, rt);

       current->state = TASK_RUN;
//	current->runtime = 0;


retry:
	next = NULL;
	wake_ns = 1000000000;
	/* XXX: for now, try to wake up any threads not running
	 * this is a waste of cycles and instruction space; should be
	 * done in the scheduler's code (somewhere) */
	list_for_each_entry(sched, &kernel_schedulers, node)
		sched->wake_next_task(&sched->tq, now);


	/* XXX need sorted list: highest->lowest scheduler priority, e.g.:
	 * EDF -> RMS -> FIFO -> RR
	 * TODO: scheduler priority value
	 */

	list_for_each_entry(sched, &kernel_schedulers, node) {



		/* if one of the schedulers have a task which needs to run now,
		 * next is non-NULL
		 */
retry2:
		next = sched->pick_next_task(&sched->tq, now);

#if 0
		if (next)
			printk("next %s %llu %llu\n", next->name, next->first_wake, ktime_get());
		else
			printk("NULL %llu\n", ktime_get());
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

		if (next) {
#if 0
			if (next->on_cpu != KTHREAD_CPU_AFFINITY_NONE) {
				if (next->on_cpu != leon3_cpuid()) {
				//	printk("%s on_cpu: %d but am %d\n", next->name, next->on_cpu, leon3_cpuid());

					if (prev == next)
						continue;

					prev = next;
					next = NULL;
					goto retry2;
				}
			//	else
			//		printk("yay %s on_cpu: %d and am %d\n", next->name, next->on_cpu, leon3_cpuid());
			}

			if (next->sched) {
#endif
				slot_ns = next->sched->timeslice_ns(next);
#if 0
				if (slot_ns < 0)
					printk("<0 ! %s\n", next->name);
			}
			else continue;
#endif
			/* we found something to execute, off we go */
			break;
		}
	}


	if (!next) {
		/* there is absolutely nothing nothing to do, check again later */
		tick_set_next_ns(wake_ns);
		kthread_unlock();
		goto exit;
	}

//	if (leon3_cpuid() != 0)
//	printk("next %s\n", next->name);
	/* see if the remaining runtime in a thread is smaller than the wakeup
	 * timeout. In this case, we will restrict ourselves to the remaining
	 * runtime. This is particularly needeed for strictly periodic
	 * schedulers, e.g. EDF
	 */

	wake_ns = sched->task_ready_ns(&sched->tq, now);

	if (wake_ns > 0)
		if (wake_ns < slot_ns)
			slot_ns  = wake_ns;

	/* ALWAYS get current time here */
	next->exec_start = ktime_get();


	/* subtract readout overhead */
	tick_set_next_ns(ktime_sub(slot_ns, 1000LL));

#if 1
	if (slot_ns < 19000UL) {
		printk("wake %lld slot %lld %s\n", wake_ns, slot_ns, next->name);
		now = ktime_get();
		goto retry;
		BUG();
	}
	sched_ev++;
	sched_last_time = ktime_add(sched_last_time, ktime_delta(ktime_get(), now));
#endif
	kthread_unlock(); 
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
