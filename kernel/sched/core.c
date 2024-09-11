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
#include <kernel/smp.h>
#include <kernel/sysctl.h>
#include <kernel/kmem.h>

#include <asm-generic/irqflags.h>
#include <asm-generic/spinlock.h>
#include <asm/switch_to.h>


#include <kernel/string.h>


#define MSG "SCHEDULER: "


/* XXX: per-cpu... */
extern struct thread_info *current_set[];

static LIST_HEAD(kernel_schedulers);
static bool sched_enabled[CONFIG_SMP_CPUS_MAX];
static uint8_t cpu_load[CONFIG_SMP_CPUS_MAX];
static struct spinlock core_spinlock[CONFIG_SMP_CPUS_MAX];

#ifdef CONFIG_CPU_STATS_COLLECT

/* we'll bomb if there are more than 99 CPUs in the system ;) */
#define CPU_MAX_CHARS_PER_NAME	2
static char cpu_load_names[CONFIG_SMP_CPUS_MAX * CPU_MAX_CHARS_PER_NAME];
static struct sobj_attribute  cpu_load_attr[CONFIG_SMP_CPUS_MAX];
static struct sobj_attribute *cpu_load_attributes[CONFIG_SMP_CPUS_MAX + 1];

static ssize_t cpu_load_show(__attribute__((unused)) struct sysobj *sobj,
			__attribute__((unused)) struct sobj_attribute *sattr,
			char *buf)
{
	int cpu;


	cpu = strtol(sattr->name, NULL, 10);

	if (cpu > CONFIG_SMP_CPUS_MAX)
		return 0;

	return sprintf(buf, "%u", sched_get_cpu_load(cpu));
}


static ssize_t proc_stats_show(__attribute__((unused)) struct sysobj *sobj,
			       __attribute__((unused)) struct sobj_attribute *sattr,
			       char *buf)
{
	struct scheduler *sched;

	struct task_struct *tsk;


	/* XXX we currently abuse "buf" to locate the particular thread
	 *
	 * TODO: - add sysobj_remove()
	 *	 - add a flag which controls whether a thread/process receives
	 *	   an entry in the sysctl tree (for speed reasons with
	 *	   one-shot thread)
	 *	 - when a dead task is removed, also remove its entry in sysctl
	 *	 - make sure to handle the (de-) allocation of name strings
	 *	 - ???
	 *	 - profit!
	 */

	/* we return the stats for the first thread of a given name in any
	 * scheduler's regular active task queue we can find; we don't care
	 * about newly added or dead tasks, these typically only exist
	 * for very brief moments
	 *
	 * note: anything value returned can only ever be considered best-effort
	 *
	 */
	list_for_each_entry(sched, &kernel_schedulers, node) {

		int i;

		struct task_struct *tmp;

		for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++) {

			list_for_each_entry_safe(tsk, tmp, &sched->tq->run, node) {

				if (!strncmp(tsk->name, buf, TASK_NAME_LEN)) {
					goto entry_found;
				}
			}
		}
	}

	return 0;

entry_found:
	if (!strcmp(sattr->name, "cpu_affinity"))
		return sprintf(buf, "%d", tsk->on_cpu);

	if (!strcmp(sattr->name, "runtime_ns"))
		return sprintf(buf, "%lld", tsk->total);

	if (!strcmp(sattr->name, "state"))
		return sprintf(buf, "0x%lx", tsk->state);

	if (!strcmp(sattr->name, "sched_policy"))
		return sprintf(buf, "%d", tsk->attr.policy);

	if (!strcmp(sattr->name, "stack_top"))
		return sprintf(buf, "0x%p", tsk->stack_top);

	if (!strcmp(sattr->name, "stack_bottom"))
		return sprintf(buf, "0x%p", tsk->stack_bottom);

	return 0;
}

__extension__
static struct sobj_attribute proc_stats_attr[] = {
       	__ATTR(cpu_affinity,  proc_stats_show, NULL),
       	__ATTR(runtime_ns,    proc_stats_show, NULL),
       	__ATTR(state,         proc_stats_show, NULL),
       	__ATTR(sched_policy,  proc_stats_show, NULL),
       	__ATTR(stack_top,     proc_stats_show, NULL),
       	__ATTR(stack_bottom,  proc_stats_show, NULL),
};

__extension__
static struct sobj_attribute *proc_stats_attributes[] = {
	&proc_stats_attr[0], &proc_stats_attr[1], &proc_stats_attr[2],
	&proc_stats_attr[3], &proc_stats_attr[4], &proc_stats_attr[5],
	NULL
};



static int  sched_init_sysctl(void)
{
	size_t i;
	struct sysobj *sobj;

	sobj = sysobj_create();

	if (!sobj)
		return -ENOMEM;

	for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++) {

		snprintf(&cpu_load_names[i * 2], CPU_MAX_CHARS_PER_NAME,"%u", i);
		cpu_load_attr[i].name  = &cpu_load_names[i * 2];
		cpu_load_attr[i].show  = cpu_load_show;
		cpu_load_attr[i].store = NULL;

		cpu_load_attributes[i] = &cpu_load_attr[i];
	}

	/* be explicit; last item in attribute pointer list is always NULL */
	cpu_load_attributes[CONFIG_SMP_CPUS_MAX] = NULL;


	sobj->sattr = cpu_load_attributes;
	sysobj_add(sobj, NULL, sysctl_root(), "cpu_load");



	/* XXX we currently export stats via a single entry and
	 * abuse the write-back buffer to transport the selection of the
	 * task name string. At some point we only want to create
	 * a new sysset entry /sys/proc here where we can pack the per-process stats
	 * when we create them
	 */
        sobj = sysobj_create();
        if (!sobj)
		return -ENOMEM;

        sobj->sattr = proc_stats_attributes;
        sysobj_add(sobj, NULL, sysctl_root(), "proc");

	return 0;
}
fs_initcall(sched_init_sysctl);

#endif /* CONFIG_CPU_STATS_COLLECT */


/**
 * @brief update the remaining runtime of the current thread and set
 *	  state from TASK_BUSY to TASK_RUN
 */

static void sched_update_runtime(struct task_struct *task, ktime now)
{
	ktime rt;

	rt = ktime_sub(now, task->exec_start);

	task->exec_stop = now;
	task->runtime = ktime_sub(task->runtime, rt);
	task->total   = ktime_add(task->total, rt);

	if (task->state == TASK_BUSY)
		task->state  = TASK_RUN;
}

/**
 * @brief find the next task to execute
 *
 * @returns the runtime to the next scheduling event
 */

static ktime sched_find_next_task(struct task_struct **task, int cpu, ktime now)
{
	struct scheduler *sched;

	struct task_struct *next;

	ktime slice;


	/* our schedulers are sorted by priority, so the highest-priority
	 * scheduler with some task to run gets to call dibs on the cpu
	 */
	list_for_each_entry(sched, &kernel_schedulers, node) {

		next = sched->pick_next_task(sched->tq, cpu, now);

		if (next) {
			/* we found something to execute, off we go */
			slice = next->sched->timeslice_ns(next);
			break;
		}
	}

	/* NOTE: _next_ can never be NULL, as there must always be at least
	 * the initial kernel bootup thread present or scheduling must be
	 * disabled altogether
	 */
	BUG_ON(!next);


	/* Determine the most pressing ready time. If the remaining runtime in
	 * a thread is smaller than the wakeup timeout for a given scheduler
	 * priority, we will restrict ourselves to the remaining runtime.
	 * This is particularly needed for strictly (periodic) schedulers,
	 * e.g. EDF
	 */
	list_for_each_entry(sched, &kernel_schedulers, node) {

		ktime ready;


		/* NOTE: non-periodic, non-real-time schedulers (e.g. round
		 *       robin, fifo, ...) are supposed to return a zero-timeout
		 *       for next task readiness, since their tasks are
		 *       always ready to run
		 */

		ready = sched->task_ready_ns(sched->tq, cpu, now);

		/* TODO raise kernel alarm if ready < 0, this would imply a
		 *	real-time requirement has been violated
		 */

		BUG_ON(ready < 0);

		if (!ready)
			continue;

		if (ready >= slice)
			continue;

		if (sched->priority >= next->sched->priority)
			slice = ready;
		else
			break;
	}


	(*task) = next;

	return slice;
}


/**
 * @brief set the load percentage of a cpu for an arbitrary time interval
 *
 * @note this is a mechanism to centrally manage the cpu loads; how loads
 *	 are updated is left to the implementation of the particular platform;
 *	 this is done on a best-effort basis for informative purposes only
 */

void sched_set_cpu_load(int cpu, uint8_t load_percent)
{
	if (cpu >= CONFIG_SMP_CPUS_MAX)
		return;

	cpu_load[cpu] = load_percent;
}


/**
 * @brief get the last known load percentage of a cpu
 *
 * @note this is provided on a best-effort basis for informative purposes only
 */

uint8_t sched_get_cpu_load(int cpu)
{
	if (cpu >= CONFIG_SMP_CPUS_MAX)
		return 0;

	return cpu_load[cpu];
}


/**
 * @brief schedule and execute the next task
 */

void schedule(void)
{
	int cpu;

	ktime now;
	ktime tick;
	ktime slice;

	struct task_struct *next;



	cpu = smp_cpu_id();

	if (!sched_enabled[cpu])
		return;

	/* booted yet? */
#if 0
	/* XXX this should be fine. It appears we can just return
	 * in case we get early high-frequency threads and a particular
	 * CPU is not fully booted
	 * TODO more tests, or boot all CPUs really early (initcall territory)
	 */

	BUG_ON(!current_set[cpu]);
#else
	if(!current_set[cpu])
		return;
#endif


	arch_local_irq_disable();
	spin_lock_raw(&core_spinlock[cpu]);

	now = ktime_get();
	sched_update_runtime(current_set[cpu]->task, now);


	tick = (ktime) tick_get_period_min_ns();

	while (1) {

		slice = sched_find_next_task(&next, cpu, now);
		if (slice > tick)
			break;

		/* keep trying until we can find a task which can actually
		 * execute given the system overhead
		 */

		now = ktime_get();
	}

	next->exec_start = now;

	/*
	 * We subtract a tick period here to account for the approximate
	 * overhead of the scheduling function, which is about twice the
	 * ISR processing time. This could be improved, but it appears
	 * to be sufficient for very high load, high frequency tasks
	 *
	 * On a GR712 @80 MHz, this makes the following EDF configuration
	 * not miss any deadlines:
	 *	1) P = 1000 ms, D = 999 ms, W = 300 ms
	 *	2) P =  140 us, D = 115 us, W =  90 us
	 *	3) P = 1000 ms, D = 999 ms, W = 300 ms
	 *	4) P =  140 us, D = 115 us, W =  90 us
	 *
	 * This configuration puts a load of 94.3% on each CPU, running tasks
	 * 1+2, 3+4 respectively. The remaining runtime is allocated to the
	 * RR-mode threads of the kernel main boot threads per CPU.
	 *
	 * Note: This means that the scheduling overhead comes out of the
	 *	 run-time budget of each task, no matter the scheduler type.
	 *	 In the above example, the high-frequency tasks cannot use
	 *	 more than about 93 percent of their actual WCET. Obviously,
	 *	 this will be significantly less for longer period/WCET tasks.
	 */

	/* set next wakeup */
	tick_set_next_ns(ktime_sub(slice, tick));

	spin_unlock(&core_spinlock[cpu]);

	/* execute switch only if needed */
	if (likely(next != current_set[cpu]->task)) {
		prepare_arch_switch(1);
		switch_to(next);
	}

	arch_local_irq_enable();
}


/**
 * @brief yield remaining runtime and reschedule
 */

void sched_yield(void)
{
	struct task_struct *tsk;

	tsk = current_set[smp_cpu_id()]->task;
	tsk->runtime = 0;

	schedule();
}


/**
 * @brief yield remaining runtime and reschedule if less than a given
 *	  fraction of the WCET remains
 */

void sched_maybe_yield(unsigned int frac_wcet)
{
	struct task_struct *tsk;


	tsk = current_set[smp_cpu_id()]->task;

	if (tsk->runtime > tsk->attr.wcet / frac_wcet) {
		tsk->runtime = 0;
		schedule();
	}
}


/**
 * @brief wake up a task
 */

int sched_wake(struct task_struct *task, ktime now)
{
	if (!task)
		return -EINVAL;

	if (!task->sched) {
		pr_err(MSG "no scheduler configured for task %s\n", task->name);
		return -EINVAL;
	}

	return task->sched->wake_task(task, now);
}


/**
 * @brief enqueue a task
 */

int sched_enqueue(struct task_struct *task)
{
	int ret;


	if (!task)
		return -EINVAL;

	if (!task->sched) {
		pr_err(MSG "no scheduler configured for task %s\n", task->name);
		return -EINVAL;
	}

	ret = task->sched->check_sched_attr(&task->attr);
	if (ret)
		return ret;

	ret = task->sched->enqueue_task(task);
	if (ret)
		return ret;

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

			task->sched = sched;

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
	struct sched_attr attr = {.policy   = SCHED_RR,
				  .priority = 100};


	return sched_set_attr(task, &attr);
}


/**
 * @brief register a new scheduler
 *
 * @returns 0 on success,
 *	   -EPERM if the scheduler instance is already registered,
 *	   -EINVAL otherwise
 *
 * XXX locking
 */

int sched_register(struct scheduler *sched)
{
	int swap;

	struct scheduler *elem;
	struct scheduler *tmp;



	if (!sched)
		return -EINVAL;


	list_for_each_entry(elem, &kernel_schedulers, node) {
		if (elem == sched) {
			pr_err(MSG "scheduler instance already registered\n");
			return -EPERM;
		}
	}

	list_add_tail(&sched->node, &kernel_schedulers);


	/* bubble-sort by priority */
	do {
		swap = 0;

		list_for_each_entry_safe(elem, tmp, &kernel_schedulers, node) {

			struct scheduler *next = list_next_entry(elem, node);

			if (elem->priority < next->priority) {

				list_swap(&elem->node, &next->node);

				swap = 1;
			}
		}

	} while (swap);



	return 0;
}


/**
 * @brief enable scheduling on the current cpu
 */

void sched_enable(void)
{
	sched_enabled[smp_cpu_id()] = true;
}


/**
 * @brief disable scheduling on the current cpu
 */
void sched_disable(void)
{
	sched_enabled[smp_cpu_id()] = false;
}


/**
 * @brief scheduler initcall
 *
 * @note sets tick mode to oneshot
 */

static int sched_init(void)
{
	tick_set_mode(TICK_MODE_ONESHOT);


	return 0;
}
subsys_initcall(sched_init);
