/**
 * @file kernel/kthread.c
 */


#include <kernel/kthread.h>
#include <kernel/export.h>
#include <kernel/smp.h>
#include <kernel/kmem.h>
#include <kernel/err.h>
#include <kernel/printk.h>

#include <asm-generic/irqflags.h>

#include <asm/io.h>
#include <asm/switch_to.h>
#include <asm/spinlock.h>

#include <kernel/string.h>

#include <kernel/tick.h>


#define MSG "KTHREAD: "

#define TASK_NAME_LEN	64

static struct spinlock kthread_spinlock;

struct thread_info *current_set[CONFIG_SMP_CPUS_MAX]; /* XXX */


/**
 * @brief lock critical kthread section
 */

static void kthread_lock(void)
{
	spin_lock_raw(&kthread_spinlock);
}


/**
 * @brief unlock critical kthread section
 */

static void kthread_unlock(void)
{
	spin_unlock(&kthread_spinlock);
}


void kthread_set_sched_edf(struct task_struct *task, unsigned long period_us,
			   unsigned long deadline_rel_us, unsigned long wcet_us)
{
	struct sched_attr attr;
	sched_get_attr(task, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = us_to_ktime(period_us);
	attr.deadline_rel = us_to_ktime(deadline_rel_us);
	attr.wcet         = us_to_ktime(wcet_us);
	sched_set_attr(task, &attr);
}



/* we should have a thread with a semaphore which is unlocked by schedule()
 * if dead tasks were added to the "dead" list
 */

void kthread_free(struct task_struct *task)
{
	return;
	if (task->flags & TASK_NO_CLEAN) /* delete from list as well */
		return;

	kfree(task->stack);
	kfree(task->name);
	kfree(task);
}


/**
 * @brief wake up a kthread
 *
 */

int kthread_wake_up(struct task_struct *task)
{
	int ret = 0;

	unsigned long flags;

	ktime now;


	if (!task)
		return -EINVAL;

	if (task->state != TASK_NEW)
		return -EINVAL;

	ret = sched_enqueue(task);
	if (ret)
		return ret;

	flags = arch_local_irq_save();
	kthread_lock();
	now = ktime_get();

	sched_wake(task, now);

	task->wakeup_first = now;

	/* this may be a critical task, send reschedule */
	if (task->on_cpu != KTHREAD_CPU_AFFINITY_NONE)
		smp_send_reschedule(task->on_cpu);

	kthread_unlock();
	arch_local_irq_restore(flags);

	return 0;
}


/**
 * @brief convert the boot path to a thread
 *
 * @note this function sets the initial task for any cpu; if a task has alreay
 *	 been set, the attempt will be rejected
 */

struct task_struct *kthread_init_main(void)
{
	int cpu;

	unsigned long flags;

	struct task_struct *task;


	cpu = smp_cpu_id();
	if (current_set[cpu])
		return ERR_PTR(-EPERM);

	task = kmalloc(sizeof(*task));
	if (!task)
		return ERR_PTR(-ENOMEM);


	sched_set_policy_default(task);

	task->state  = TASK_NEW;
	task->name   = strdup("KERNEL");
	task->on_cpu = cpu;

	arch_promote_to_task(task);

	flags = arch_local_irq_save();
	kthread_lock();

	current_set[cpu] = &task->thread_info;

	sched_enqueue(task);
	sched_wake(task, ktime_get());

	smp_send_reschedule(cpu);

	kthread_unlock();
	arch_local_irq_restore(flags);

	return task;
}


/**
 * @brief create a new thread
 */

static struct task_struct *kthread_create_internal(int (*thread_fn)(void *data),
						   void *data, int cpu,
						   const char *namefmt,
						   va_list args)
{
	struct task_struct *task;


	task = kzalloc(sizeof(struct task_struct));
	if (!task)
		return ERR_PTR(-ENOMEM);

	/* NOTE: we require that malloc always returns properly aligned memory,
	 * i.e. aligned to the largest possible memory access instruction
	 * (which is typically 64 bits)
	 */

	task->stack = kmalloc(CONFIG_STACK_SIZE);
	if (!task->stack) {
		kfree(task);
		return ERR_PTR(-ENOMEM);
	}

	/* initialise stack with pattern, makes detection of errors easier */
	memset32(task->stack, 0xdeadbeef, CONFIG_STACK_SIZE / sizeof(uint32_t));

	task->stack_bottom = task->stack;
	task->stack_top    = (void *) ((uint8_t *) task->stack
						   + CONFIG_STACK_SIZE);

	task->name = kmalloc(TASK_NAME_LEN);
	vsnprintf(task->name, TASK_NAME_LEN, namefmt, args);

	if (sched_set_policy_default(task)) {
		pr_crit("KTHREAD: task policy error\n");
		kthread_free(task);
		return NULL;
	}

	task->create = ktime_get();
	task->total  = 0;
	task->slices = 0;
	task->on_cpu = cpu;
	task->state  = TASK_NEW;

	arch_init_task(task, thread_fn, data);

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



