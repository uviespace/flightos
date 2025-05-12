#include <syscalls.h>
#include <thread.h>




void thread_set_sched_edf(thread_t *t,
			  unsigned long period_us,
			  unsigned long wcet_us,
			  unsigned long deadline_rel_us)
{
	if (!t)
		return;

	t->attr.policy       = KSCHED_EDF;
	t->attr.period       = period_us;
	t->attr.wcet         = wcet_us;
	t->attr.deadline_rel = deadline_rel_us;
}




int thread_create(thread_t *t, int (*thread_fn)(void *data),
		  void *data, int cpu, char *name)
{
	if (!t)
		return -1;

	if (!thread_fn)
		return -1;


	t->size      = sizeof(thread_t);
	t->thread_fn = thread_fn;
	t->data      = data;
	t->name      = name;

	t->attr.on_cpu = cpu;

	return 0;
}


int thread_wake_up(thread_t *t)
{
	return sys_thread_create(t);
}


void sched_yield(void)
{
	sys_sched_yield();
}
