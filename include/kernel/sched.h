/**
 * @file include/kernel/sched.h
 * @ingroup schedthread
 * @ingroup schedsys
 *
 * @brief High-level scheduler policies, task queues, and scheduler contract.
 */


#ifndef _KERNEL_SCHED_H_
#define _KERNEL_SCHED_H_

#include <list.h>
#include <kernel/time.h>

#include <generated/autoconf.h>	/*XXX */

/**
 * @brief scheduler priority levels
 */
#define KSCHED_PRIORITY_RR	0	/*!< round-robin scheduler priority */
#define KSCHED_PRIORITY_EDF	1	/*!< EDF scheduler priority */

/**
 * @brief scheduling policies
 */
enum ksched_policy {
	KSCHED_RR,		/*!< round-robin scheduling */
	KSCHED_EDF,		/*!< earliest deadline first scheduling */
	KSCHED_OTHER,		/*!< other/default scheduling policy */
};


/**
 * @brief scheduling attributes for a task
 */
struct sched_attr {
	enum ksched_policy	policy;		/*!< scheduling policy */

	unsigned long		priority;	/*!< static priority for RR/FIFO */

	ktime			period __attribute__ ((aligned (8)));		/*!< wakeup period for EDF/RMS */
	ktime			wcet __attribute__ ((aligned (8)));		/*!< max runtime per period */
	ktime			deadline_rel __attribute__ ((aligned (8)));	/*!< relative deadline from wakeup */

}  __attribute__ ((aligned (8)));





#if 0
struct rq {

		/* runqueue lock: */
	raw_spinlock_t		lock;

	struct dl_rq		dl;
};
#endif


/**
 * @brief task queue holding lists for different task states
 */
struct task_queue {
	struct list_head wake;	/*!< tasks waiting to be woken */
	struct list_head run;	/*!< currently runnable tasks */
	struct list_head dead;	/*!< terminated tasks */
	struct list_head clean;	/*!< tasks pending cleanup */
};







#if 1
/**
 * @brief a pluggable scheduler instance
 */
struct scheduler {

	struct task_queue	tq[CONFIG_SMP_CPUS_MAX]; /*!< per-CPU task queues */

	const enum ksched_policy policy;		/*!< scheduler policy */

	struct task_struct *(*pick_next_task)(struct task_queue tq[], int cpu,
					      ktime now);	/*!< pick next runnable task */

	int (*wake_task)    (struct task_struct *task, ktime now);	/*!< wake a task */
	int (*enqueue_task) (struct task_struct *task);			/*!< enqueue a task */

	ktime (*timeslice_ns)   (struct task_struct *task);	/*!< get task timeslice */
	ktime (*task_ready_ns)  (struct task_queue tq[], int cpu, ktime now);	/*!< get ready time */

	int (*check_sched_attr) (struct sched_attr *attr);	/*!< validate sched attributes */

	unsigned long priority;		/*!< scheduler priority */
	struct list_head	node;	/*!< node in scheduler list */
#if 0
	const struct sched_class *next;

	void (*enqueue_task) (struct rq *rq, struct task_struct *p, int flags);
	void (*dequeue_task) (struct rq *rq, struct task_struct *p, int flags);
	void (*yield_task)   (struct rq *rq);
	bool (*yield_to_task)(struct rq *rq, struct task_struct *p, bool preempt);

	void (*check_preempt_curr)(struct rq *rq, struct task_struct *p, int flags);
	/*
	 * It is the responsibility of the pick_next_task() method that will
	 * return the next task to call put_prev_task() on the @prev task or
	 * something equivalent.
	 *
	 * May return RETRY_TASK when it finds a higher prio class has runnable
	 * tasks.
	 */
	struct task_struct * (*pick_next_task)(struct rq *rq,
					       struct task_struct *prev,
					       struct rq_flags *rf);
#endif
#if 0
	void (*put_prev_task)(struct rq *rq, struct task_struct *p);
#endif

};
#endif


/**
 * @brief switch CPU context to the given task
 *
 * @param next the task to switch to
 */
void switch_to(struct task_struct *next);

/**
 * @brief run the scheduler to switch to the next runnable task
 */
void schedule(void);

/**
 * @brief voluntarily yield the CPU to another task
 */
void sched_yield(void);

/**
 * @brief conditionally yield the CPU if the task has used the given fraction
 *        of its worst-case execution time
 * @param frac_wcet: fraction of the wcet as a percentage
 */
void sched_maybe_yield(unsigned int frac_wcet);


/**
 * @brief set the scheduling attributes of a task
 * @param task: the task to configure
 * @param attr: the scheduling attributes to apply
 * @return 0 on success, negative error code on failure
 */
int sched_set_attr(struct task_struct *task, struct sched_attr *attr);

/**
 * @brief get the scheduling attributes of a task
 * @param task: the task to query
 * @param attr: output buffer for the scheduling attributes
 * @return 0 on success, negative error code on failure
 */
int sched_get_attr(struct task_struct *task, struct sched_attr *attr);


/**
 * @brief set a task's policy to the default scheduler policy
 * @param task: the task to configure
 * @return 0 on success, negative error code on failure
 */
int sched_set_policy_default(struct task_struct *task);

/**
 * @brief enqueue a task to be scheduled
 * @param task: the task to enqueue
 * @return 0 on success, negative error code on failure
 */
int sched_enqueue(struct task_struct *task);

/**
 * @brief wake a (sleeping) task at the given time
 * @param task: the task to wake
 * @param now: the current time
 * @return 0 on success, negative error code on failure
 */
int sched_wake(struct task_struct *task, ktime now);

/**
 * @brief register a scheduler instance with the kernel
 * @param sched: the scheduler to register
 * @return 0 on success, negative error code on failure
 */
int sched_register(struct scheduler *sched);


/**
 * @brief enable scheduling
 */
void sched_enable(void);

/**
 * @brief disable scheduling
 */
void sched_disable(void);


/**
 * @brief set the load of a CPU
 * @param cpu: the CPU id
 * @param load_percent: the load in percent
 */
void sched_set_cpu_load(int cpu, uint8_t load_percent);

/**
 * @brief get the load of a CPU
 * @param cpu: the CPU id
 * @return the load in percent
 */
uint8_t sched_get_cpu_load(int cpu);


#endif /* _KERNEL_SCHED_H_ */
