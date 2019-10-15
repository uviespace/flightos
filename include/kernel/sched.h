/**
 * @file include/kernel/sched.h
 */


#ifndef _KERNEL_SCHED_H_
#define _KERNEL_SCHED_H_

#include <generated/autoconf.h>	/*XXX */



enum sched_policy {
	SCHED_RR,
	SCHED_EDF,
	SCHED_FIFO,
	SCHED_OTHER,
};


struct sched_attr {
	enum sched_policy	policy;

	/* static priority scheduling for RR, FIFO, ... */
	unsigned long		priority;

	/* period based scheduling for EDF, RMS, ... */
	ktime			period;		/* wakeup period */
	ktime			wcet;		/* max runtime per period*/
	ktime			deadline_rel;	/* time to deadline from begin of wakeup */
};





#if 0
struct rq {

		/* runqueue lock: */
	raw_spinlock_t		lock;

	struct dl_rq		dl;
};
#endif


struct task_queue {
	struct list_head new;
	struct list_head run;
	struct list_head wake;
	struct list_head dead;
};







#if 1
struct scheduler {

	struct task_queue	tq[CONFIG_SMP_CPUS_MAX]; /* XXX */

	const enum sched_policy policy;

	struct task_struct *(*pick_next_task)(struct task_queue tq[], int cpu,
					      ktime now);

	/* XXX: sucks */
	void (*wake_next_task)  (struct task_queue tq[], int cpu, ktime now);
	void (*enqueue_task)    (struct task_queue tq[],
			         struct task_struct *task);

	ktime (*timeslice_ns)   (struct task_struct *task);
	ktime (*task_ready_ns)  (struct task_queue tq[], int cpu, ktime now);

	int (*check_sched_attr) (struct sched_attr *attr);

	unsigned long sched_priority;		/* scheduler priority */
	struct list_head	node;
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





int sched_set_attr(struct task_struct *task, struct sched_attr *attr);
int sched_get_attr(struct task_struct *task, struct sched_attr *attr);

int sched_set_policy_default(struct task_struct *task);
int sched_enqueue(struct task_struct *task);
int sched_register(struct scheduler *sched);


#endif /* _KERNEL_SCHED_H_ */
