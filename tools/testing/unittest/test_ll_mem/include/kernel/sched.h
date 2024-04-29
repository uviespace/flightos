/**
 * @file include/kernel/sched.h
 */


#ifndef _KERNEL_SCHED_H_
#define _KERNEL_SCHED_H_

#include <list.h>
#include <kernel/time.h>

#include <generated/autoconf.h>	/*XXX */

/* scheduler priority levels */
#define SCHED_PRIORITY_RR	0
#define SCHED_PRIORITY_EDF	1

enum sched_policy {
	SCHED_RR,
	SCHED_EDF,
	SCHED_OTHER,
};



struct sched_attr {
	enum sched_policy	policy;

	/* static priority scheduling for RR, FIFO, ... */
	unsigned long		priority;

	/* period based scheduling for EDF, RMS, ... */
	ktime			period __attribute__ ((aligned (8)));		/* wakeup period */
	ktime			wcet __attribute__ ((aligned (8)));		/* max runtime per period*/
	ktime			deadline_rel __attribute__ ((aligned (8)));	/* time to deadline from begin of wakeup */

}  __attribute__ ((aligned (8)));





#if 0
struct rq {

		/* runqueue lock: */
	raw_spinlock_t		lock;

	struct dl_rq		dl;
};
#endif


struct task_queue {
	struct list_head wake;
	struct list_head run;
	struct list_head dead;
};







#if 1
struct scheduler {

	struct task_queue	tq[CONFIG_SMP_CPUS_MAX]; /* XXX */

	const enum sched_policy policy;

	struct task_struct *(*pick_next_task)(struct task_queue tq[], int cpu,
					      ktime now);

	int (*wake_task)    (struct task_struct *task, ktime now);
	int (*enqueue_task) (struct task_struct *task);

	ktime (*timeslice_ns)   (struct task_struct *task);
	ktime (*task_ready_ns)  (struct task_queue tq[], int cpu, ktime now);

	int (*check_sched_attr) (struct sched_attr *attr);

	unsigned long priority;		/* scheduler priority */
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


void switch_to(struct task_struct *next);
void schedule(void);
void sched_yield(void);
void sched_maybe_yield(unsigned int frac_wcet);


int sched_set_attr(struct task_struct *task, struct sched_attr *attr);
int sched_get_attr(struct task_struct *task, struct sched_attr *attr);

int sched_set_policy_default(struct task_struct *task);
int sched_enqueue(struct task_struct *task);
int sched_wake(struct task_struct *task, ktime now);
int sched_register(struct scheduler *sched);

void sched_enable(void);
void sched_disable(void);

void sched_set_cpu_load(int cpu, uint8_t load_percent);
uint8_t sched_get_cpu_load(int cpu);


#endif /* _KERNEL_SCHED_H_ */
