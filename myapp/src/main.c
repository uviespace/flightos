#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <thread.h>
#include <signals.h>
#include <time.h>
#include <sysctl.h>


static int create_realtime_thread(int (*cyc_func)(void *arg),
				      unsigned int period_ms,
				      unsigned int runtime_ms,
				      unsigned int deadline_ms)
{
	static thread_t th;


	if (!cyc_func)
		return -1;

	thread_create(&th, cyc_func, NULL, 0, "RT_THREAD");

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

uint32_t diff_timespec_ms(const struct timespec *t1,
				 const struct timespec *t0)
{
	struct timespec delta;


	delta = diff_timespec(t1, t0);

	return delta.tv_nsec / 1000000 + delta.tv_sec * 1000;
}


uint32_t diff_timespec_us(const struct timespec *t1,
				 const struct timespec *t0)
{
	struct timespec delta;


	delta = diff_timespec(t1, t0);

	return delta.tv_nsec / 1000 + delta.tv_sec * 1000000;
}



static int thread_func(void *arg __attribute__((unused)))
{
	struct timespec t1;
	struct timespec t0;


	while (1) {

		clock_gettime(CLOCK_REALTIME, &t0);
#if 0
		execute_test_function_here();
#endif
		clock_gettime(CLOCK_REALTIME, &t1);
		printf("took %lu us\n", diff_timespec_us(&t1, &t0));

#if 0		/* can just quit here */
		return 0;
#endif
		/* we always yield remaining RT for more accurate timings */
		sched_yield();
	}
}


/* RT thread configuration: 90% of CPU WCET */
#define PERIOD_MS	1000
#define RUNTIME_MS	 900
#define DEADLINE_MS	 999

int main(void)
{
	create_realtime_thread(thread_func, PERIOD_MS, RUNTIME_MS, DEADLINE_MS);

	/* thread started, we can exit */

	return EXIT_SUCCESS;
}
