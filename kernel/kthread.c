/**
 * @file kernel/kthread.c
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



#define MSG "KTHREAD: "


static struct {
	struct list_head new;
	struct list_head run;
	struct list_head wake;
	struct list_head dead;
} _kthreads = {
	.new  = LIST_HEAD_INIT(_kthreads.new),
	.run  = LIST_HEAD_INIT(_kthreads.run),
	.wake = LIST_HEAD_INIT(_kthreads.wake),
	.dead = LIST_HEAD_INIT(_kthreads.dead)
};

static struct spinlock kthread_spinlock;


/** XXX dummy **/
struct task_struct *kernel;


struct thread_info *current_set[1];


/**
 * @brief lock critical kthread section
 */

static inline void kthread_lock(void)
{
	spin_lock(&kthread_spinlock);
}


/**
 * @brief unlock critical kthread section
 */

static inline void kthread_unlock(void)
{
	spin_unlock(&kthread_spinlock);
}


/* this should be a thread with a semaphore
 * that is unlocked by schedule() if dead tasks
 * were added
 * (need to irq_disable/kthread_lock)
 */

void kthread_cleanup_dead(void)
{
	struct task_struct *p_elem;
	struct task_struct *p_tmp;

	list_for_each_entry_safe(p_elem, p_tmp, &_kthreads.dead, node) {
		list_del(&p_elem->node);
		kfree(p_elem->stack);
		kfree(p_elem->name);
		kfree(p_elem);
	}
}



void sched_yield(void)
{
	struct task_struct *tsk;

	tsk = current_set[0]->task;
	if (tsk->policy == SCHED_EDF)
		tsk->runtime = 0;

	schedule();
}


void sched_wake(struct task_struct *next, ktime now, int64_t slot_ns)
{

	struct task_struct *task;

	if (list_empty(&_kthreads.wake))
		return;

	task = list_entry(_kthreads.wake.next, struct task_struct, node);

	if (task->policy == SCHED_EDF) {
		if (next->policy == SCHED_EDF)
			return;
		/* initially set current time as wakeup */
		task->wakeup = ktime_add(now, slot_ns);
		task->deadline = ktime_add(task->wakeup, task->deadline_rel);
		task->first_wake = task->wakeup;
		task->first_dead = task->deadline;

		list_move(&task->node, &_kthreads.run);
	}

	if (task->policy == SCHED_RR) {
		task->state = TASK_RUN;
		list_move(&task->node, &_kthreads.run);
	}

}


#define MIN_SLICE	1000000LL	/* 1 ms */

#define OVERHEAD	0LL

void schedule(void)
{
	struct task_struct *next;
	struct task_struct *current;
	int64_t slot_ns = MIN_SLICE;



	ktime now;


	if (list_empty(&_kthreads.run))
		return;



	/* XXX: dummy; need solution for sched_yield() so timer is
	 * disabled either via disable_tick or need_resched variable
	 * (in thread structure maybe?)
	 * If we don't do this here, the timer may fire while interrupts
	 * are disabled, which may land us here yet again
	 */


	arch_local_irq_disable();

	kthread_lock();

	now = ktime_add(ktime_get(), OVERHEAD);

	tick_set_next_ns(1e9);



	current = current_set[0]->task;



	if (current->policy == SCHED_EDF) {
		ktime d;
		d = ktime_delta(now, current->exec_start);
		BUG_ON(d < 0);
		current->total = ktime_add(current->total, d);
		current->slices++;
		current->runtime = ktime_sub(current->runtime, d);
		//printk("%lld %lld\n", d, current->runtime);
	}


	/** XXX not here, add cleanup thread */
	kthread_cleanup_dead();


#if 1
	{
		static int init;
		if (!init) { /* dummy switch */
			tick_set_mode(TICK_MODE_ONESHOT);
			init = 1;
		}
	}
#endif








#if 0
	slot_ns = schedule_edf(now);
#endif


	/* round robin as before */
	do {

		next = list_entry(_kthreads.run.next, struct task_struct, node);

		BUG_ON(!next);

		if (next->state == TASK_RUN) {
			list_move_tail(&next->node, &_kthreads.run);
			break;
		}
		if (next->state == TASK_IDLE) {
			list_move_tail(&next->node, &_kthreads.run);
			continue;
		}

		if (next->state == TASK_DEAD)
			list_move_tail(&next->node, &_kthreads.dead);

	} while (!list_empty(&_kthreads.run));


	if (next->policy == SCHED_EDF) {
		if (next->runtime <= slot_ns)
			slot_ns = next->runtime; /* XXX must track actual time because of IRQs */
	}

	if (next->policy == SCHED_RR)
		slot_ns = (ktime) next->priority * MIN_SLICE;



	if (slot_ns > 0xffffffff)
		slot_ns = 0xffffffff;

	BUG_ON (slot_ns < 18000);


	sched_wake(next, now, slot_ns);

	next->exec_start = ktime_get();

	kthread_unlock();

	tick_set_next_ns(slot_ns);

	prepare_arch_switch(1);
	switch_to(next);

	arch_local_irq_enable();
}

__attribute__((unused))
static void kthread_set_sched_policy(struct task_struct *task,
				     enum sched_policy policy)
{
	arch_local_irq_disable();
	kthread_lock();
	task->policy = policy;
	kthread_unlock();
	arch_local_irq_enable();
}



void kthread_wake_up(struct task_struct *task)
{
	arch_local_irq_disable();
	kthread_lock();

	BUG_ON(task->state != TASK_NEW);

	task->state = TASK_IDLE;

	list_move_tail(&task->node, &_kthreads.wake);

	kthread_unlock();
	arch_local_irq_enable();
}



struct task_struct *kthread_init_main(void)
{
	struct task_struct *task;

	task = kmalloc(sizeof(*task));

	if (!task)
		return ERR_PTR(-ENOMEM);

	/* XXX accessors */
	task->policy = SCHED_RR; /* default */
	task->priority = 1;

	arch_promote_to_task(task);

	task->name = "KERNEL";
	task->policy = SCHED_RR;

	arch_local_irq_disable();
	kthread_lock();

	kernel = task;
	/*** XXX dummy **/
	current_set[0] = &kernel->thread_info;


	task->state = TASK_RUN;
	list_add_tail(&task->node, &_kthreads.run);



	kthread_unlock();
	arch_local_irq_enable();

	return task;
}




static struct task_struct *kthread_create_internal(int (*thread_fn)(void *data),
						   void *data, int cpu,
						   const char *namefmt,
						   va_list args)
{
	struct task_struct *task;

	task = kmalloc(sizeof(*task));


	if (!task)
		return ERR_PTR(-ENOMEM);


	/* XXX: need stack size detection and realloc/migration code */

	task->stack = kmalloc(8192 + STACK_ALIGN); /* XXX */

	BUG_ON((int) task->stack > (0x40800000 - 4096 + 1));

	if (!task->stack) {
		kfree(task);
		return ERR_PTR(-ENOMEM);
	}


	task->stack_bottom = task->stack; /* XXX */
	task->stack_top = ALIGN_PTR(task->stack, STACK_ALIGN) +8192/4; /* XXX */
	BUG_ON(task->stack_top > (task->stack + (8192/4 + STACK_ALIGN/4)));

#if 0
	/* XXX: need wmemset() */
	memset(task->stack, 0xab, 8192 + STACK_ALIGN);
#else
	{
		int i;
		for (i = 0; i < (8192 + STACK_ALIGN) / 4; i++)
			((int *) task->stack)[i] = 0xdeadbeef;

	}
#endif

	/* dummy */
	task->name = kmalloc(32);
	BUG_ON(!task->name);
	vsnprintf(task->name, 32, namefmt, args);

	/* XXX accessors */
	task->policy = SCHED_RR; /* default */
	task->priority = 1;

	task->total = 0;
	task->slices = 0;
	arch_init_task(task, thread_fn, data);

	task->state = TASK_NEW;

	arch_local_irq_disable();
	kthread_lock();

	list_add_tail(&task->node, &_kthreads.new);

	kthread_unlock();
	arch_local_irq_enable();

	return task;
}




/**
 * @brief create a new kernel thread
 *
 * @param thread_fn the function to run in the thread
 * @param data a user data pointer for thread_fn, may be NULL
 *
 * @param cpu set the cpu affinity
 *
 * @param name_fmt a printf format string name for the thread
 *
 * @param ... parameters to the format string
 */

struct task_struct *kthread_create(int (*thread_fn)(void *data),
				   void *data, int cpu,
				   const char *namefmt,
				   ...)
{
	struct task_struct *task;
	va_list args;

	va_start(args, namefmt);
	task = kthread_create_internal(thread_fn, data, cpu, namefmt, args);
	va_end(args);

	return task;
}
EXPORT_SYMBOL(kthread_create);



