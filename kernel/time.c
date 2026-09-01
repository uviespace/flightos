/**
 * @file kernel/time.c
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 *
 * @ingroup time
 * @ingroup timing
 * @defgroup time time interface
 *
 * @brief Generic timekeeping and time-value operations.
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
 *
 * @return the readout overhead in nanoseconds
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
 *
 * @return a struct timespec with the current kernel time
 */

struct timespec get_ktime(void) __attribute__((alias("get_uptime")));
EXPORT_SYMBOL(get_ktime);












/**
 * @brief returns the number of seconds elapsed between time1 and time0
 *
 * @param time1 a struct timespec
 * @param time0 a struct timespec
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
 * @param time1 a struct timespec
 * @param time0 a struct timespec
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

	pr_info(MSG "calibrated main uptime clock readout overhead to %d ns\n",
	            tk.readout_ns);
}

/**
 * @brief normalise a struct timespec so 0 <= tv_nsec < NSEC_PER_SEC
 *
 * @param t the struct timespec to normalise
 *
 * @return the normalised struct timespec
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
 *
 * @param t1 the first struct timespec to add
 * @param t2 the second struct timespec to add
 *
 * @return the normalised sum of t1 and t2
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
 * @param nsec the number of nanoseconds to convert
 *
 * @return the corresponding struct timespec
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
 * @param ts the struct timespec to add to
 * @param nsec the number of nanoseconds to add
 *
 * @return the resulting struct timespec
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
 * @param  nsec nanoseconds
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


/**
 * @brief convert a ktime to a struct timespec
 *
 * @param t the ktime value to convert
 *
 * @return the corresponding struct timespec
 */

inline struct timespec ktime_to_timespec(ktime t)
{
	return ns_to_timespec(t);
}

/**
 * @brief converts a struct timespec to ktime_t
 *
 * @param ts the struct timespec to convert
 *
 * @return the corresponding ktime value
 */

inline ktime timespec_to_ktime(struct timespec ts)
{
	return ktime_set(ts.tv_sec, ts.tv_nsec);
}


/**
 * @brief compare ktimes
 *
 * @param t1 first ktime value
 * @param t2 second ktime value
 *
 * @return < 0 if t1  < t2
 *           0 if t1 == t2
 *         > 0 if t1  > t2
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
 * @brief check if a ktime t1 is after t2
 *
 * @param t1 first ktime value
 * @param t2 second ktime value
 *
 * @return true if t1 was after t2, false otherwise
 */

inline bool ktime_after(const ktime t1, const ktime t2)
{
	return ktime_compare(t1, t2) > 0;
}


/**
 * @brief check if a ktime t1 is before t2
 *
 * @param t1 first ktime value
 * @param t2 second ktime value
 *
 * @return true if t1 was before t2, false otherwise
 */

inline bool ktime_before(const ktime t1, const ktime t2)
{
	return ktime_compare(t1, t2) < 0;
}


/**
 * @brief add two ktime values
 *
 * @param t1 first ktime value
 * @param t2 second ktime value
 *
 * @return the sum of t1 and t2
 */

inline ktime ktime_add(const ktime t1, const ktime t2)
{
	return t1 + t2;
}


/**
 * @brief subtract two ktime values
 *
 * @param later   the later ktime value
 * @param earlier the earlier ktime value
 *
 * @return the difference in nanoseconds
 */

inline ktime ktime_sub(const ktime later, const ktime earlier)
{
	return later - earlier;
}



/**
 * @brief add nanoseconds to a ktime
 *
 * @param t    the ktime value
 * @param nsec nanoseconds to add
 *
 * @return the resulting ktime
 */

inline ktime ktime_add_ns(const ktime t, const uint64_t nsec)
{
	return t + nsec;
}


/**
 * @brief subtract nanoseconds from a ktime
 *
 * @param t    the ktime value
 * @param nsec nanoseconds to subtract
 *
 * @return the resulting ktime
 */

inline ktime ktime_sub_ns(const ktime t, const uint64_t nsec)
{
	return t - nsec;
}


/**
 * @brief add microseconds to a ktime
 *
 * @param t    the ktime value
 * @param usec microseconds to add
 *
 * @return the resulting ktime
 */

inline ktime ktime_add_us(const ktime t, const uint64_t usec)
{
	return ktime_add_ns(t, usec * NSEC_PER_USEC);
}


/**
 * @brief add milliseconds to a ktime
 *
 * @param t    the ktime value
 * @param msec milliseconds to add
 *
 * @return the resulting ktime
 */

inline ktime ktime_add_ms(const ktime t, const uint64_t msec)
{
	return ktime_add_ns(t, msec * NSEC_PER_MSEC);
}


/**
 * @brief subtract microseconds from a ktime
 *
 * @param t    the ktime value
 * @param usec microseconds to subtract
 *
 * @return the resulting ktime
 */

inline ktime ktime_sub_us(const ktime t, const uint64_t usec)
{
	return ktime_sub_ns(t, usec * NSEC_PER_USEC);
}


/**
 * @brief subtract milliseconds from a ktime
 *
 * @param t    the ktime value
 * @param msec milliseconds to subtract
 *
 * @return the resulting ktime
 */

inline ktime ktime_sub_ms(const ktime t, const uint64_t msec)
{
	return ktime_sub_ns(t, msec * NSEC_PER_MSEC);
}

/**
 * @brief calculate the delta between two ktime values
 *
 * @param later   the later ktime value
 * @param earlier the earlier ktime value
 *
 * @return the difference in nanoseconds
 */

inline int64_t ktime_delta(const ktime later, const ktime earlier)
{
       return ktime_sub(later, earlier);
}

/**
 * @brief calculate the delta between two ktime values in microseconds
 *
 * @param later   the later ktime value
 * @param earlier the earlier ktime value
 *
 * @return the difference in microseconds
 */

inline int64_t ktime_us_delta(const ktime later, const ktime earlier)
{
       return ktime_to_us(ktime_sub(later, earlier));
}


/**
 * @brief calculate the delta between two ktime values in milliseconds
 *
 * @param later   the later ktime value
 * @param earlier the earlier ktime value
 *
 * @return the difference in milliseconds
 */

inline int64_t ktime_ms_delta(const ktime later, const ktime earlier)
{
	return ktime_to_ms(ktime_sub(later, earlier));
}


/**
 * @brief convert a ktime to microseconds
 *
 * @param t the ktime value
 *
 * @return the value in microseconds
 */

inline int64_t ktime_to_us(const ktime t)
{
	return t / (int64_t) NSEC_PER_USEC;
}


/**
 * @brief convert a ktime to milliseconds
 *
 * @param t the ktime value
 *
 * @return the value in milliseconds
 */

inline int64_t ktime_to_ms(const ktime t)
{
	return t / (int64_t) NSEC_PER_MSEC;
}

/**
 * @brief convert microseconds to ktime
 *
 * @param usec the value in microseconds
 *
 * @return the corresponding ktime value
 */

inline ktime us_to_ktime(const int64_t usec)
{
	return (ktime) (usec * (int64_t) NSEC_PER_USEC);
}

/**
 * @brief convert milliseconds to ktime
 *
 * @param msec the value in milliseconds
 *
 * @return the corresponding ktime value
 */

inline ktime ms_to_ktime(const int64_t msec)
{
	return (ktime) (msec * (int64_t) NSEC_PER_MSEC);
}



/**
 * @brief get current kernel time (== uptime)
 *
 * @return the current kernel time in nanoseconds as a ktime value
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
 * @brief (re)initialise the timing system
 *
 * @param clock pointer to the clocksource to use for timekeeping
 */

void time_init(struct clocksource *clock)
{
	tk.clock = clock;
	time_init_overhead_calibrate();
}
