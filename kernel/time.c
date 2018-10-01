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
 *
 * XXX we really need a 64 bit ktime_t or overflows are very likely here
 * TODO need software implementation of 64 bit add/sub/mult
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
 * @brief normalise a struct timespec so 0 <= tv_nsec < NSEC_PER_SEC
 */

struct timespec timespec_normalise(struct timespec t)
{
	while (t.tv_nsec >= NSEC_PER_SEC) {
		t.tv_sec++;
		t.tv_nsec -= NSEC_PER_SEC;
	}

	while (t.tv_nsec < 0) {
		t.tv_sec--;
		t.tv_nsec += NSEC_PER_SEC;
	}

	return t;
}


/**
 * @brief add two struct timespec
 */

struct timespec timespec_add(struct timespec t1, struct timespec t2)
{
	t1.tv_sec  += t2.tv_sec;
	t1.tv_nsec += t2.tv_nsec;

	return timespec_normalise(t1);
}
EXPORT_SYMBOL(timespec_add);

/**
 * @brief convert nanoseconds to a struct timespec
 *
 * @warn this is guaranteed to produce incorrect results for
 *	  nsec >= 2^31 * 10^9 if sizeof(tv_sec) == 4
 */

struct timespec ns_to_timespec(const int64_t nsec)
{
	struct timespec ts = {0};


	if (!nsec)
		return ts;


	BUG_ON(nsec > (~(1LL << 31) * NSEC_PER_SEC));

	ts.tv_sec  = (typeof(ts.tv_sec))  (nsec / (int64_t) NSEC_PER_SEC);

	ts.tv_nsec = (typeof(ts.tv_nsec)) (nsec - (int64_t) (ts.tv_sec
							     * NSEC_PER_SEC));

	return timespec_normalise(ts);
}
EXPORT_SYMBOL(ns_to_timespec);


/**
 * @brief add nanoseconds to a timespec
 *
 * @warn this is guaranteed to produce incorrect results for
 *	  nsec >= 2^32 * 10^9 if sizeof(tv_sec) == 4
 */

struct timespec timespec_add_ns(struct timespec ts, int64_t nsec)
{

	if (!nsec)
		return ts;

	return timespec_add(ts, ns_to_timespec(nsec));
}
EXPORT_SYMBOL(timespec_add_ns);


/**
 * @brief  set a ktime from a seconds and nanoseconds
 * @param  sec	 seconds
 * @@param nsec nanoseconds
 *
 * @returns the ktime representation of the input values
 *
 * @note this allows to set ktime to be at most 10^9 * 2 * 2^32 =
 *       4294967300294967296 ns ~ 136 years which is probably enough :)
 */
inline ktime ktime_set(const unsigned long sec, const unsigned long nsec)
{
	return (ktime) sec * NSEC_PER_SEC + (ktime) nsec;
}


inline struct timespec ktime_to_timespec(ktime t)
{
	return ns_to_timespec(t);
}

/**
 * @brief converts a struct timespec to ktime_t
 */

inline ktime timespec_to_ktime(struct timespec ts)
{
	return ktime_set(ts.tv_sec, ts.tv_nsec);
}


/**
 * @brief compare ktimes
 *
 * @returns < 0 if t1  < t2
 *            0 if t1 == t2
 *          > 0 if t1  > t2
 */

int ktime_compare(const ktime t1, const ktime t2)
{
	if (t1 < t2)
		return -1;

	if (t1 > t2)
		return 1;

	return 0;
}


/**
 * @param  see if a ktime t1 is after t2
 * Return: true if t1 was after t2, false otherwise
 */

inline bool ktime_after(const ktime t1, const ktime t2)
{
	return ktime_compare(t1, t2) > 0;
}


/**
 * @param  see if a ktime t1 is before t2
 * Return: true if t1 was before t2, false otherwise
 */

inline bool ktime_before(const ktime t1, const ktime t2)
{
	return ktime_compare(t1, t2) < 0;
}


inline ktime ktime_add(const ktime t1, const ktime t2)
{
	return t1 + t2;
}


inline ktime ktime_sub(const ktime later, const ktime earlier)
{
	return later - earlier;
}



inline ktime ktime_add_ns(const ktime t, const uint64_t nsec)
{
	return t + nsec;
}


inline ktime ktime_sub_ns(const ktime t, const uint64_t nsec)
{
	return t - nsec;
}


inline ktime ktime_add_us(const ktime t, const uint64_t usec)
{
	return ktime_add_ns(t, usec * NSEC_PER_USEC);
}


inline ktime ktime_add_ms(const ktime t, const uint64_t msec)
{
	return ktime_add_ns(t, msec * NSEC_PER_MSEC);
}


inline ktime ktime_sub_us(const ktime t, const uint64_t usec)
{
	return ktime_sub_ns(t, usec * NSEC_PER_USEC);
}


inline ktime ktime_sub_ms(const ktime t, const uint64_t msec)
{
	return ktime_sub_ns(t, msec * NSEC_PER_MSEC);
}

inline int64_t ktime_delta(const ktime later, const ktime earlier)
{
       return ktime_sub(later, earlier);
}

inline int64_t ktime_us_delta(const ktime later, const ktime earlier)
{
       return ktime_to_us(ktime_sub(later, earlier));
}


inline int64_t ktime_ms_delta(const ktime later, const ktime earlier)
{
	return ktime_to_ms(ktime_sub(later, earlier));
}


inline int64_t ktime_to_us(const ktime t)
{
	return t / (int64_t) NSEC_PER_USEC;
}


inline int64_t ktime_to_ms(const ktime t)
{
	return t / (int64_t) NSEC_PER_MSEC;
}

inline ktime us_to_ktime(const int64_t usec)
{
	return (ktime) (usec * (int64_t) NSEC_PER_USEC);
}

inline ktime ms_to_ktime(const int64_t msec)
{
	return (ktime) (msec * (int64_t) NSEC_PER_MSEC);
}



/**
 * @brief get current kernel time (== uptime)
 *
 * @note if no uptime clock was configured, the result will be 0
 */

ktime ktime_get(void)
{
	ktime ns;

	uint32_t sec;
	uint32_t nsec;



	if (!tk.clock)
		return 0;

	tk.clock->read(&sec, &nsec);

	ns = (ktime) sec * NSEC_PER_SEC + (ktime) nsec;

	ns += (ktime) ktime_get_readout_overhead();


	return ns;
}
EXPORT_SYMBOL(ktime_get);







/**
 * @brief initialise the timing system
 */

void time_init(struct clocksource *clock)
{
	tk.clock = clock;
	time_init_overhead_calibrate();
}
