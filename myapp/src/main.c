#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <thread.h>
#include <time.h>  

static int launch_cyclical_activities(int (*cyc_func)(void *arg),
				      unsigned int period_ms,
				      unsigned int runtime_ms,
				      unsigned int deadline_ms)
{
	static thread_t th;

	
	if (!cyc_func)
		return -1;

	thread_create(&th, cyc_func, NULL, 0, "CYC_ACT");

	thread_set_sched_edf(&th, period_ms   * 1000,
				  runtime_ms  * 1000,
				  deadline_ms * 1000);

	return thread_wake_up(&th);
}


static struct timespec diff_timespec(const struct timespec *t1,
				     const struct timespec *t0)
{
	struct timespec delta;


	delta.tv_sec  = t1->tv_sec  - t0->tv_sec;
	delta.tv_nsec = t1->tv_nsec - t0->tv_nsec;

	if (delta.tv_nsec < 0) {
		delta.tv_nsec += 1000000000;
		delta.tv_sec--;
	}

	return delta;
}

static uint32_t diff_timespec_ms(const struct timespec *t1,
				 const struct timespec *t0)
{
	struct timespec delta;


	delta = diff_timespec(t1, t0);

	return delta.tv_nsec / 1000000 + delta.tv_sec * 1000;
}


static int asw_cycle(void *arg __attribute__((unused)))
{
	struct timespec t1;
	struct timespec t0 = {0};


	while (1) {
		clock_gettime(CLOCK_REALTIME, &t1);

		printf("delta %lu ms, up %g s\n", diff_timespec_ms(&t1, &t0),
			(double) t1.tv_sec + (double) t1.tv_nsec * 1e-9);

		clock_gettime(CLOCK_REALTIME, &t0);
		sched_yield();
	}

	return 0;
}


/* the default EDF thread configuration for the ASW */
#define ASW_PERIOD_MS		500
#define ASW_RUNTIME_MS		400	/* the WCET */
#define ASW_DEADLINE_MS		490

int main(void)
{
	launch_cyclical_activities(asw_cycle,
				   ASW_PERIOD_MS,
				   ASW_RUNTIME_MS,
				   ASW_DEADLINE_MS);

        while (1)
                sched_yield();

	return EXIT_SUCCESS;
}
