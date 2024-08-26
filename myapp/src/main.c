#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <thread.h>
#include <time.h>

unsigned int busy;


static inline unsigned long leon3_cpuid(void)
{
        unsigned long cpuid;

        __asm__ __volatile__ (
                        "rd     %%asr17, %0     \n\t"
                        "srl    %0, 28, %0      \n\t"
                        :"=r" (cpuid)
                        :
                        :"l1");
        return cpuid;
}


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

#pragma GCC push_options
#pragma GCC optimize ("O0")
static unsigned char call_depth(unsigned char value, size_t level, int sp)
{
	register int sp_local asm("%sp");

	if (!level) {
		printf("d_sp: %4d ", sp - sp_local);
		return 0;
	}

	return value + call_depth(value, level - 1, sp);
}
#pragma GCC pop_options

static int compress(void *data)
{
	size_t r;

	unsigned int i;

	register int sp_local asm("%sp");

	unsigned char *buf;



	buf = (unsigned char *) data;

	for (i = 0; i < 1024*10 + (busy - 1); i++) {
		if (buf[i] != (i & 0xff))
			printf("NO MATCH! %d %d\n", i, buf[i]);
	}

	r = (rand() + busy) % 22;

	printf("%d/%3d lvl %2d (CPU %ld)\n", busy, call_depth(buf[i - 1], r, sp_local), r, leon3_cpuid());

	free(data);

	busy = 0;

	return 0;
}


static int asw_cycle(void *arg __attribute__((unused)))
{
	struct timespec t1;
	struct timespec t0 = {0};

	thread_t th;

	char *buf1;
	char *buf;
	int i;
	int cnt = 0;


	while (1) {

		if (busy)
			sched_yield();

		clock_gettime(CLOCK_REALTIME, &t1);

		printf("dt %03lu ms, up %.5f s CPU %ld ", diff_timespec_ms(&t1, &t0),
			(double) t1.tv_sec + (double) t1.tv_nsec * 1e-9, leon3_cpuid());

		buf1 = malloc(1024*1024);
		if (buf1) {
			for (i = 0; i < 1024*1024; i++)
				buf1[i] = i & 0xff;
		}


		buf = malloc(1024*10 + cnt);
		if (buf) {
			for (i = 0; i < 1024*10 + cnt; i++)
				buf[i] = i & 0xff;
			free(buf1);
			buf1 = NULL;
			cnt++;
		}

		if (!busy && buf) {
			busy = cnt;

			th.attr.policy = SCHED_RR;
			th.attr.priority = 10000;
			thread_create(&th, compress, buf, 1, "SCI_COMPR");
			thread_wake_up(&th);
			buf = NULL;
		} else {
			printf("STILL BUSY %d\n", busy);
		}

		clock_gettime(CLOCK_REALTIME, &t0);
		sched_yield();
	}

	return 0;
}


/* the default EDF thread configuration for the ASW */
#define ASW_PERIOD_MS		500/8
#define ASW_RUNTIME_MS		400/8	/* the WCET */
#define ASW_DEADLINE_MS		490/8

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
