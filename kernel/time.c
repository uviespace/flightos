/**
 * @file kernel/ktime.c
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 *
 * @ingroup time
 * @defgroup time time interface
 *
 */


#include <errno.h>
#include <kernel/time.h>
#include <kernel/export.h>

#define MSG "KTIME: "

static struct timekeeper tk;


/**
 * @brief returns the readout overhead of the uptime/ktime clock
 *	  in nanoseconds
 *
 * @note this is a self-calibrated value
 */

uint32_t ktime_get_readout_overhead(void)
{
	return tk.readout_ns;
}
EXPORT_SYMBOL(ktime_get_readout_overhead);


/**
 * @brief get the time elapsed since boot
 *
 * @return struct timespec
 *
 * @note if no uptime clock was configured, the result will be 0
 */

struct timespec get_uptime(void)
{
	uint32_t sec;
	uint32_t nsec;

	struct timespec ts = {0};


	if (!tk.clock)
		return ts;


	tk.clock->read(&sec, &nsec);

	/* We'll get away with this as long as we exist in 32-bit space, since
	 * the members of struct timespec are usually of long int type.
	 * (see also kernel/time.h)
	 */

	ts.tv_sec  = (typeof(ts.tv_sec)) sec;
	ts.tv_nsec = (typeof(ts.tv_sec)) nsec;

	return ts;
}
EXPORT_SYMBOL(get_uptime);


/**
 * @brief get the current kernel time
 * @note for now, this is just an alias of get_uptime
 */

struct timespec get_ktime(void) __attribute__((alias("get_uptime")));
EXPORT_SYMBOL(get_ktime);



/**
 * @brief returns the number of seconds elapsed between time1 and time0
 *
 * @param ts1 a struct timespec
 * @param ts2 a struct timespec
 *
 * @returns the time delta in seconds, represented as double
 */

double difftime(const struct timespec time1, const struct timespec time0)
{
	double t0, t1;

	t0 = (double) time0.tv_sec + (double) time0.tv_nsec * 1e-9;
	t1 = (double) time1.tv_sec + (double) time1.tv_nsec * 1e-9;

	return t1 - t0;
}
EXPORT_SYMBOL(difftime);


/**
 * @brief returns the number of nanoseconds elapsed between time1 and time0
 *
 * @param ts1 a struct timespec
 * @param ts2 a struct timespec
 *
 * @returns the time delta in nanoseconds, represented as double
 */

double difftime_ns(const struct timespec time1, const struct timespec time0)
{
	return difftime(time1, time0) * 1e9;
}
EXPORT_SYMBOL(difftime_ns);




static void time_init_overhead_calibrate(void)
{
#define CALIBRATE_LOOPS 100
	int i;

	double delta = 0.0;

	struct timespec t0;


	for (i = 0; i < CALIBRATE_LOOPS; i++) {
		t0 = get_ktime();
		delta += difftime_ns(get_ktime(), t0);
	}

	/* overhead is readout delta / 2 */
	tk.readout_ns = (typeof(tk.readout_ns)) (0.5 * delta / (double) i);

	printk(MSG "calibrated main uptime clock readout overhead to %d ns\n",
	           tk.readout_ns);
}


/**
 * @brief initialise the timing system
 */

void time_init(struct clocksource *clock)
{
	tk.clock = clock;
	time_init_overhead_calibrate();
}
