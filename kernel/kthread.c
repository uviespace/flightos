/**
 * @file kernel/kthread.c
 */


#include <kernel/kthread.h>
#include <kernel/export.h>
#include <kernel/kmem.h>
#include <kernel/err.h>


static inline unsigned int get_psr(void)
{
	unsigned int psr;
	__asm__ __volatile__(
		"rd	%%psr, %0\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
	: "=r" (psr)
	: /* no inputs */
	: "memory");

	return psr;
}

static inline void put_psr(unsigned int new_psr)
{
	__asm__ __volatile__(
		"wr	%0, 0x0, %%psr\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
	: /* no outputs */
	: "r" (new_psr)
	: "memory", "cc");
}



struct task_struct *kernel;


struct {
	struct task_struct *current;
	struct task_struct *second;
	struct task_struct *third;
} tasks;


struct thread_info *current_set[1]; // = {kernel->thread_info};


#define prepare_arch_switch(next) do { \
	__asm__ __volatile__( \
	"save %sp, -0x40, %sp; save %sp, -0x40, %sp; save %sp, -0x40, %sp\n\t" \
	"save %sp, -0x40, %sp; save %sp, -0x40, %sp; save %sp, -0x40, %sp\n\t" \
	"save %sp, -0x40, %sp\n\t" \
	"restore; restore; restore; restore; restore; restore; restore"); \
} while(0)



void schedule(void)
{
	struct task_struct *tmp;

	if (tasks.current && tasks.second && tasks.third)
		(void) 0;
	else
		return;


	tmp = tasks.current;
	if (tasks.second) {
		tasks.current = tasks.second;
		if (tasks.third) {
			tasks.second = tasks.third;
			tasks.third = tmp;
		} else {
			tasks.second = tmp;
		}
	} else {
		return;
	}

//	printk("new task: %p\n", tasks.current);


	prepare_arch_switch(1);
#if 0
	__asm__ __volatile__("/*#define curptr    g6*/"
			   "sethi	%%hi(here - 0x8), %%o7\n\t"	/* save the program counter just at the jump below as calĺed return address*/
			    "or	%%o7, %%lo(here - 0x8), %%o7\n\t"       /* so the old thread will hop over this section when it returns */
			   "rd	%%psr, %%g4\n\t"
			   "std %%sp, [%%g6 + %2]\n\t" //store %sp and skip %pc to current thread's KSP
			   "rd	%%wim, %%g5\n\t"	// read wim
			   "wr	%%g4, 0x00000020, %%psr\n\t" // toggle ET bit (should be off at this point!
			   "nop\n\t"
			   "nop\n\t"
			   "nop\n\t"
			   //? pause
			   "std	%%g4, [%%g6 + %4]\n\t"	// store %psr to KPSR and %wim to KWIM

			   "ldd	[%1 + %4], %%g4\n\t"	// load KPSR + KWIM into %g4, %g5 from new thread
			   "mov	%1, %%g6\n\t"		// and set the new thread as current
			   "st	%1, [%0]\n\t"		// and to current_set[]
			   "wr	%%g4, 0x20, %%psr\n\t"	// set new PSR and toggle ET (should be off)
			   "nop; nop; nop\n\t"		// wait for bits to settle, so we are in the proper window
			   "ldd	[%%g6 + %2], %%sp\n\t"	// and and load KSP to %sp (%o6) and KPC to %o7 (all of these MUST be aligned to dbl)
			   "wr	%%g5, 0x0, %%wim\n\t"	//set the new KWIM (from double load above)

			   "ldd	[%%sp + 0x00], %%l0\n\t" //load  %l0 (%t_psr and %pc
			   "ldd	[%%sp + 0x38], %%i6\n\t"	// load %fp and %i7 (return address)
			   "wr	%%g4, 0x0, %%psr\n\t"		// set the original PSR (without traps)
			   "jmpl %%o7 + 0x8, %%g0\n\t"		// as the thread is switched in, it will jump to the "here" marker and continue
				"nop\n"
			   "here:\n"
			   :
			   : "r" (&(current_set[0])),
			     "r" (&(tasks.next->thread_info)),
				"i" (TI_KSP),
				"i" (TI_KPC),
				"i" (TI_KPSR)
					:       "g1", "g2", "g3", "g4", "g5",       "g7",
				"l0", "l1",       "l3", "l4", "l5", "l6", "l7",
				"i0", "i1", "i2", "i3", "i4", "i5",
				"o0", "o1", "o2", "o3",                   "o7");
#else
	__asm__ __volatile__("/*#define curptr    g6*/"
			   "sethi	%%hi(here - 0x8), %%o7\n\t"	/* save the program counter just at the jump below as calĺed return address*/
			    "or	%%o7, %%lo(here - 0x8), %%o7\n\t"       /* so the old thread will hop over this section when it returns */
			   "rd	%%psr, %%g4\n\t"
			   "std %%sp, [%%g6 + %2]\n\t" //store %sp and skip %pc to current thread's KSP
			   "rd	%%wim, %%g5\n\t"	// read wim
			   "wr	%%g4, 0x00000020, %%psr\n\t" // toggle ET bit (should be off at this point!
			   "nop\n\t"
			   "nop\n\t"
			   "nop\n\t"
			   "std	%%g4, [%%g6 + %4]\n\t"	// store %psr to KPSR and %wim to KWIM
			   "ldd	[%1 + %4], %%g4\n\t"	// load KPSR + KWIM into %g4, %g5 from new thread
			   "mov	%1, %%g6\n\t"		// and set the new thread as current
			   "st	%1, [%0]\n\t"		// and to current_set[]
			   "wr	%%g4, 0x20, %%psr\n\t"	// set new PSR and toggle ET (should be off)
			   "nop; nop; nop\n\t"		// wait for bits to settle, so we are in the proper window
			   "ldd	[%%g6 + %2], %%sp\n\t"	// and and load KSP to %sp (%o6) and KPC to %o7 (all of these MUST be aligned to dbl)
			   "wr	%%g5, 0x0, %%wim\n\t"	//set the new KWIM (from double load above)

			   "ldd	[%%sp + 0x00], %%l0\n\t" //load  %l0 (%t_psr and %pc
			   "ldd	[%%sp + 0x38], %%i6\n\t"	// load %fp and %i7 (return address)
			   "wr	%%g4, 0x0, %%psr\n\t"		// set the original PSR (without traps)
			   "jmpl %%o7 + 0x8, %%g0\n\t"		// as the thread is switched in, it will jump to the "here" marker and continue
				"nop\n"
			   "here:\n"
			   :
			   : "r" (&(current_set[0])),
			     "r" (&(tasks.current->thread_info)),
				"i" (TI_KSP),
				"i" (TI_KPC),
				"i" (TI_KPSR)
					:       "g1", "g2", "g3", "g4", "g5",       "g7",
				"l0", "l1",       "l3", "l4", "l5", "l6", "l7",
				"i0", "i1", "i2", "i3", "i4", "i5",
				"o0", "o1", "o2", "o3",                   "o7");
#endif


}


#define curptr g6

/* this is executed from an interrupt exit  */
void __attribute__((always_inline)) switch_to(struct task_struct *next)
{
	//struct task_struct *task;
	//struct thread_info *ti;

	printk("Switch!\n");
	prepare_arch_switch(1);




	/* NOTE: we don't actually require the PSR_ET toggle, but if we have
	 * unaligned accesses (or access traps), it is a really good idea, or we'll die */
	/* NOTE: this assumes we have a mixed kernel/user mapping in the MMU (if
	 * we are using it), otherwise we might would not be able to load the
	 * thread's data. Oh, and we'll have to do a switch user->kernel->new
	 * user OR we'll run into the same issue with different user contexts */

	/* first, store the current thread */
#if 0
	__asm__ __volatile__("/*#define curptr    g6*/"
			   "sethi	%%hi(here - 0x8), %%o7\n\t"	/* save the program counter just at the jump below as calĺed return address*/
			    "or	%%o7, %%lo(here - 0x8), %%o7\n\t"       /* so the old thread will hop over this section when it returns */
			   "rd	%%psr, %%g4\n\t"
			   "std %%sp, [%%g6 + %2]\n\t" //store %sp and skip %pc to current thread's KSP
			   "rd	%%wim, %%g5\n\t"	// read wim
			   "wr	%%g4, 0x00000020, %%psr\n\t" // toggle ET bit (should be off at this point!
			   "nop\n\t"
			   //? pause
			   "std	%%g4, [%%g6 + %4]\n\t"	// store %psr to KPSR and %wim to KWIM

			   "ldd	[%1 + %4], %%g4\n\t"	// load KPSR + KWIM into %g4, %g5 from new thread
			   "mov	%1, %%g6\n\t"		// and set the new thread as current
			   "st	%1, [%0]\n\t"		// and to current_set[]
			   "wr	%%g4, 0x20, %%psr\n\t"	// set new PSR and toggle ET (should be off)
			   "nop; nop; nop\n\t"		// wait for bits to settle, so we are in the proper window
			   "ldd	[%%g6 + %2], %%sp\n\t"	// and and load KSP to %sp (%o6) and KPC to %o7 (all of these MUST be aligned to dbl)
			   "wr	%%g5, 0x0, %%wim\n\t"	//set the new KWIM (from double load above)

			   "ldd	[%%sp + 0x00], %%l0\n\t" //load  %l0 (%t_psr and %pc
			   "ldd	[%%sp + 0x38], %%i6\n\t"	// load %fp and %i7 (return address)
			   "wr	%%g4, 0x0, %%psr\n\t"		// set the original PSR (without traps)
			   "jmpl %%o7 + 0x8, %%g0\n\t"		// as the thread is switched in, it will jump to the "here" marker and continue
				"nop\n"
			   "here:\n"
			   :
			   : "r" (&(current_set[0])),
			     "r" (&(next->thread_info)),
				"i" (TI_KSP),
				"i" (TI_KPC),
				"i" (TI_KPSR)
					:       "g1", "g2", "g3", "g4", "g5",       "g7",
				"l0", "l1",       "l3", "l4", "l5", "l6", "l7",
				"i0", "i1", "i2", "i3", "i4", "i5",
				"o0", "o1", "o2", "o3",                   "o7");

#endif

}

#if 0

       __asm__ __volatile__(
                       "mov %0, %%fp      \n\t"
                       "sub %%fp, 96, %%sp\n\t"
                       :
                       : "r" (task->stack)
                       : "memory");

       thread_fn(data);
#endif

#include <asm/leon.h>

void kthread_wake_up(struct task_struct *task)
{
	printk("running thread function\n");

	task->thread_fn(task->data);
}

__attribute__((unused))
static void kthread_exit(void)
{
	printk("thread leaving\n");
}

struct task_struct *kthread_init_main(void)
{
	struct task_struct *task;

	task = kmalloc(sizeof(*task));

	if (!task)
		return ERR_PTR(-ENOMEM);

	/*** XXX dummy **/
	current_set[0] = &kernel->thread_info;


#define PSR_CWP     0x0000001f

	task->thread_info.ksp = (unsigned long) leon_get_fp();
	task->thread_info.kpc = (unsigned long) __builtin_return_address(1) - 8;
	task->thread_info.kpsr = get_psr();
	task->thread_info.kwim = 1 << (((get_psr() & PSR_CWP) + 1) % 8);
	task->thread_info.task = task;

	task->thread_fn = NULL;
	task->data      = NULL;


		printk("kernel stack %x\n", leon_get_fp());
	/* dummy */
	tasks.current = task;

	__asm__ __volatile__("mov	%0, %%g6\n\t"
			     :: "r"(&(tasks.current->thread_info)) : "memory");		// and set the new thread as current

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

	task->stack = kmalloc(8192) + 8192; /* XXX */

	if (!task->stack) {
		kfree(task);
		return ERR_PTR(-ENOMEM);
	}


#define STACKFRAME_SZ	96
#define PTREG_SZ	80
#define PSR_CWP     0x0000001f
	task->thread_info.ksp = (unsigned long) task->stack - (STACKFRAME_SZ + PTREG_SZ);
	task->thread_info.kpc = (unsigned long) thread_fn - 8;
	task->thread_info.kpsr = get_psr();
	task->thread_info.kwim = 1 << (((get_psr() & PSR_CWP) + 1) % 8);
	task->thread_info.task = task;

	task->thread_fn = thread_fn;
	task->data      = data;


		printk("%s is next at %p stack %p\n", namefmt, &task->thread_info, task->stack);
		if (!tasks.second)
			tasks.second = task;
		else
			tasks.third = task;

	/* wake up */



#if 0
	struct kthread_create_info *create = kmalloc(sizeof(*create),
						     GFP_KERNEL);

	if (!create)
		return ERR_PTR(-ENOMEM);
	create->threadfn = threadfn;
	create->data = data;
	create->node = node;
	create->done = &done;
	spin_lock(&kthread_create_lock);
	list_add_tail(&create->list, &kthread_create_list);
	spin_unlock(&kthread_create_lock);

	wake_up_process(kthreadd_task);
	/*
	 * Wait for completion in killable state, for I might be chosen by
	 * the OOM killer while kthreadd is trying to allocate memory for
	 * new kernel thread.
	 */
	if (unlikely(wait_for_completion_killable(&done))) {
		/*
		 * If I was SIGKILLed before kthreadd (or new kernel thread)
		 * calls complete(), leave the cleanup of this structure to
		 * that thread.
		 */
		if (xchg(&create->done, NULL))
			return ERR_PTR(-EINTR);
		/*
		 * kthreadd (or new kernel thread) will call complete()
		 * shortly.
		 */
		wait_for_completion(&done);
	}
	task = create->result;
	if (!IS_ERR(task)) {
		static const struct sched_param param = { .sched_priority = 0 };

		vsnprintf(task->comm, sizeof(task->comm), namefmt, args);
		/*
		 * root may have changed our (kthreadd's) priority or CPU mask.
		 * The kernel thread should not inherit these properties.
		 */
		sched_setscheduler_nocheck(task, SCHED_NORMAL, &param);
		set_cpus_allowed_ptr(task, cpu_all_mask);
	}
	kfree(create);
#endif
	return task;
}





/**
 * try_to_wake_up - wake up a thread
 * @p: the thread to be awakened
 * @state: the mask of task states that can be woken
 * @wake_flags: wake modifier flags (WF_*)
 *
 * If (@state & @p->state) @p->state = TASK_RUNNING.
 *
 * If the task was not queued/runnable, also place it back on a runqueue.
 *
 * Atomic against schedule() which would dequeue a task, also see
 * set_current_state().
 *
 * Return: %true if @p->state changes (an actual wakeup was done),
 *	   %false otherwise.
 */
static int
wake_up_thread_internal(struct task_struct *p, unsigned int state, int wake_flags)
{
	//unsigned long flags;
	//int cpu = 0;
	int success = 0;
#if 0
	/*
	 * If we are going to wake up a thread waiting for CONDITION we
	 * need to ensure that CONDITION=1 done by the caller can not be
	 * reordered with p->state check below. This pairs with mb() in
	 * set_current_state() the waiting thread does.
	 */
	raw_spin_lock_irqsave(&p->pi_lock, flags);
	smp_mb__after_spinlock();
	if (!(p->state & state))
		goto out;

	trace_sched_waking(p);

	/* We're going to change ->state: */
	success = 1;
	cpu = task_cpu(p);

	/*
	 * Ensure we load p->on_rq _after_ p->state, otherwise it would
	 * be possible to, falsely, observe p->on_rq == 0 and get stuck
	 * in smp_cond_load_acquire() below.
	 *
	 * sched_ttwu_pending()                 try_to_wake_up()
	 *   [S] p->on_rq = 1;                  [L] P->state
	 *       UNLOCK rq->lock  -----.
	 *                              \
	 *				 +---   RMB
	 * schedule()                   /
	 *       LOCK rq->lock    -----'
	 *       UNLOCK rq->lock
	 *
	 * [task p]
	 *   [S] p->state = UNINTERRUPTIBLE     [L] p->on_rq
	 *
	 * Pairs with the UNLOCK+LOCK on rq->lock from the
	 * last wakeup of our task and the schedule that got our task
	 * current.
	 */
	smp_rmb();
	if (p->on_rq && ttwu_remote(p, wake_flags))
		goto stat;

#ifdef CONFIG_SMP
	/*
	 * Ensure we load p->on_cpu _after_ p->on_rq, otherwise it would be
	 * possible to, falsely, observe p->on_cpu == 0.
	 *
	 * One must be running (->on_cpu == 1) in order to remove oneself
	 * from the runqueue.
	 *
	 *  [S] ->on_cpu = 1;	[L] ->on_rq
	 *      UNLOCK rq->lock
	 *			RMB
	 *      LOCK   rq->lock
	 *  [S] ->on_rq = 0;    [L] ->on_cpu
	 *
	 * Pairs with the full barrier implied in the UNLOCK+LOCK on rq->lock
	 * from the consecutive calls to schedule(); the first switching to our
	 * task, the second putting it to sleep.
	 */
	smp_rmb();

	/*
	 * If the owning (remote) CPU is still in the middle of schedule() with
	 * this task as prev, wait until its done referencing the task.
	 *
	 * Pairs with the smp_store_release() in finish_task().
	 *
	 * This ensures that tasks getting woken will be fully ordered against
	 * their previous state and preserve Program Order.
	 */
	smp_cond_load_acquire(&p->on_cpu, !VAL);

	p->sched_contributes_to_load = !!task_contributes_to_load(p);
	p->state = TASK_WAKING;

	if (p->in_iowait) {
		delayacct_blkio_end(p);
		atomic_dec(&task_rq(p)->nr_iowait);
	}

	cpu = select_task_rq(p, p->wake_cpu, SD_BALANCE_WAKE, wake_flags);
	if (task_cpu(p) != cpu) {
		wake_flags |= WF_MIGRATED;
		set_task_cpu(p, cpu);
	}

#else /* CONFIG_SMP */

	if (p->in_iowait) {
		delayacct_blkio_end(p);
		atomic_dec(&task_rq(p)->nr_iowait);
	}

#endif /* CONFIG_SMP */

	ttwu_queue(p, cpu, wake_flags);
stat:
	ttwu_stat(p, cpu, wake_flags);
out:
	raw_spin_unlock_irqrestore(&p->pi_lock, flags);
#endif
	return success;
}

/**
 * wake_up_process - Wake up a specific process
 * @p: The process to be woken up.
 *
 * Attempt to wake up the nominated process and move it to the set of runnable
 * processes.
 *
 * Return: 1 if the process was woken up, 0 if it was already running.
 *
 * It may be assumed that this function implies a write memory barrier before
 * changing the task state if and only if any tasks are woken up.
 */
/* Used in tsk->state: */
#define TASK_RUNNING			0x0000
#define TASK_INTERRUPTIBLE		0x0001
#define TASK_UNINTERRUPTIBLE		0x0002
#define __TASK_STOPPED			0x0004
#define __TASK_TRACED			0x0008

#define TASK_NORMAL			(TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)
int wake_up_thread(struct task_struct *p)
{
	return wake_up_thread_internal(p, TASK_NORMAL, 0);
}
EXPORT_SYMBOL(wake_up_thread);


/**
 *
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
 *
 * Create a named kernel thread. The thread will be initially stopped.
 * Use wake_up_thread to activate it.
 *
 * If cpu is set to KTHREAD_CPU_AFFINITY_NONE, the thread will be affine to all
 * CPUs. IF the selected CPU index excceds the number of available CPUS, it
 * will default to KTHREAD_CPU_AFFINITY_NONE, otherwise the thread will be
 * bound to that CPU
 *
 * The new thread has SCHED_NORMAL policy.
 *
 * If thread is going to be bound on a particular cpu, give its node
 * in @node, to get NUMA affinity for kthread stack, or else give NUMA_NO_NODE.
 * When woken, the thread will run @threadfn() with @data as its
 * argument. @threadfn() can either call do_exit() directly if it is a
 * standalone thread for which no one will call kthread_stop(), or
 * return when 'kthread_should_stop()' is true (which means
 * kthread_stop() has been called).  The return value should be zero
 * or a negative error number; it will be passed to kthread_stop().
 *
 * Returns a task_struct or ERR_PTR(-ENOMEM) or ERR_PTR(-EINTR).
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
