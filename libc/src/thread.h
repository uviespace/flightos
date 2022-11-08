#ifndef THREAD_H
#define THREAD_H

#include <stddef.h>



#define THREAD_CPU_AFFINITY_NONE	(-1)

enum sched_policy {
	SCHED_RR,
	SCHED_EDF,
	SCHED_OTHER
};

struct sched_attr {

	/* period based scheduling for EDF, RMS, ... */
	unsigned long		period;           /* wakeup period */
	unsigned long		wcet;             /* max runtime per period*/
	unsigned long		deadline_rel;     /* time to deadline from begin of wakeup */

	/* static priority scheduling for RR, FIFO, ... */
	unsigned long		priority;

	int			on_cpu;						/* cpu number or THREAD_CPU_AFFINITY_NONE */
	enum sched_policy	policy;
    };

typedef struct {
	size_t  size;
	void	*th;				/* reference to the kernel thread object */
	int	(*thread_fn)(void *data);
	void	*data;
	char	*name;
	struct sched_attr attr;
} thread_t;


int thread_wake_up(thread_t *t);

int thread_create(thread_t *t, int (*thread_fn)(void *data),
		  void *data, int cpu, char *name);

void thread_set_sched_edf(thread_t *t,
			  unsigned long period_us,
			  unsigned long wcet_us,
			  unsigned long deadline_rel_us);

void sched_yield(void);

#endif /* THREAD_H */
