/**
 * @file    include/kernel/time.h
 * @ingroup timing
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @brief High-level time representation, conversion, and timekeeping interface.
 *
 * @ingroup time
 *
 * @copyright GPLv2
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef _KERNEL_KTIME_H_
#define _KERNEL_KTIME_H_

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/clocksource.h>

#if 0
#if (INTPTR_MAX == INT32_MAX)
/* we use the compiler-defined struct timespec at this time, but we can
 * at least verify the size of the types to see if we are compatible
 */
compile_time_assert((member_size(struct timespec, tv_sec)  == sizeof(int32_t)),
		    TIMESPEC_SEC_SIZE_MISMATCH);
compile_time_assert((member_size(struct timespec, tv_nsec) == sizeof(int32_t)),
		    TIMESPEC_NSEC_SIZE_MISMATCH);
#endif /* (INTPTR_MAX == INT32_MAX) */
#endif
#define MSEC_PER_SEC	      1000L
#define USEC_PER_MSEC	      1000L
#define NSEC_PER_USEC	      1000L
#define NSEC_PER_MSEC	   1000000L
#define USEC_PER_SEC	   1000000L
#define NSEC_PER_SEC	1000000000L

#define KTIME_MAX	(~(1LL << 63))
#define KTIME_SEC_MAX	(KTIME_MAX / NSEC_PER_SEC)

/* ktime is nanoseconds since boot */
typedef int64_t ktime;




/**
 * @brief timekeeper structure linking a clocksource to its overhead
 */
struct timekeeper {
	struct clocksource *clock;	/*!< the underlying clocksource */
	uint32_t readout_ns;		/*!< readout time overhead in nanoseconds */
};


/**
 * @brief get the current time in ktime (nanoseconds since boot)
 * @return current time as ktime value
 */
ktime ktime_get(void);

/**
 * @brief create a ktime value from seconds and nanoseconds
 * @param sec: seconds component
 * @param nsec: nanoseconds component
 * @return resulting ktime value
 */
ktime ktime_set(const unsigned long sec, const unsigned long nsec);

/**
 * @brief convert a timespec to ktime
 * @param ts: the timespec to convert
 * @return resulting ktime value
 */
ktime timespec_to_ktime(struct timespec ts);

/**
 * @brief add two ktime values
 * @param t1: first time
 * @param t2: second time
 * @return sum of t1 and t2
 */
ktime ktime_add(const ktime t1, const ktime t2);

/**
 * @brief subtract two ktime values
 * @param later: the later time (minuend)
 * @param earlier: the earlier time (subtrahend)
 * @return difference (later - earlier)
 */
ktime ktime_sub(const ktime later, const ktime earlier);

/**
 * @brief add nanoseconds to a ktime value
 * @param t: base time
 * @param nsec: nanoseconds to add
 * @return resulting ktime value
 */
ktime ktime_add_ns(const ktime t, const uint64_t nsec);

/**
 * @brief subtract nanoseconds from a ktime value
 * @param t: base time
 * @param nsec: nanoseconds to subtract
 * @return resulting ktime value
 */
ktime ktime_sub_ns(const ktime t, const uint64_t nsec);

/**
 * @brief add microseconds to a ktime value
 * @param t: base time
 * @param usec: microseconds to add
 * @return resulting ktime value
 */
ktime ktime_add_us(const ktime t, const uint64_t usec);

/**
 * @brief add milliseconds to a ktime value
 * @param t: base time
 * @param msec: milliseconds to add
 * @return resulting ktime value
 */
ktime ktime_add_ms(const ktime t, const uint64_t msec);

/**
 * @brief subtract microseconds from a ktime value
 * @param t: base time
 * @param usec: microseconds to subtract
 * @return resulting ktime value
 */
ktime ktime_sub_us(const ktime t, const uint64_t usec);

/**
 * @brief subtract milliseconds from a ktime value
 * @param t: base time
 * @param msec: milliseconds to subtract
 * @return resulting ktime value
 */
ktime ktime_sub_ms(const ktime t, const uint64_t msec);

/**
 * @brief compare two ktime values
 * @param t1: first time
 * @param t2: second time
 * @return negative if t1 < t2, 0 if equal, positive if t1 > t2
 */
int ktime_compare(const ktime t1, const ktime t2);

/**
 * @brief check if t1 is strictly after t2
 * @param t1: first time
 * @param t2: second time
 * @return true if t1 > t2, false otherwise
 */
bool ktime_after(const ktime t1, const ktime t2);

/**
 * @brief check if t1 is strictly before t2
 * @param t1: first time
 * @param t2: second time
 * @return true if t1 < t2, false otherwise
 */
bool ktime_before(const ktime t1, const ktime t2);

/**
 * @brief compute the delta between two ktime values in nanoseconds
 * @param later: the later time
 * @param earlier: the earlier time
 * @return difference in nanoseconds
 */
int64_t ktime_delta(const ktime later, const ktime earlier);

/**
 * @brief compute the delta between two ktime values in microseconds
 * @param later: the later time
 * @param earlier: the earlier time
 * @return difference in microseconds
 */
int64_t ktime_us_delta(const ktime later, const ktime earlier);

/**
 * @brief compute the delta between two ktime values in milliseconds
 * @param later: the later time
 * @param earlier: the earlier time
 * @return difference in milliseconds
 */
int64_t ktime_ms_delta(const ktime later, const ktime earlier);

/**
 * @brief convert ktime to microseconds
 * @param t: ktime value to convert
 * @return time in microseconds
 */
int64_t ktime_to_us(const ktime t);

/**
 * @brief convert ktime to milliseconds
 * @param t: ktime value to convert
 * @return time in milliseconds
 */
int64_t ktime_to_ms(const ktime t);

/**
 * @brief convert microseconds to ktime
 * @param usec: microseconds to convert
 * @return resulting ktime value
 */
ktime us_to_ktime(const int64_t usec);

/**
 * @brief convert milliseconds to ktime
 * @param msec: milliseconds to convert
 * @return resulting ktime value
 */
ktime ms_to_ktime(const int64_t msec);


/**
 * @brief add two timespec values
 * @param t1: first timespec
 * @param t2: second timespec
 * @return resulting timespec (t1 + t2)
 */
struct timespec timespec_add(struct timespec t1, struct timespec t2);

/**
 * @brief convert nanoseconds to a timespec
 * @param nsec: nanoseconds to convert
 * @return resulting timespec
 */
struct timespec ns_to_timespec(const int64_t nsec);

/**
 * @brief add nanoseconds to a timespec
 * @param ts: base timespec
 * @param nsec: nanoseconds to add
 * @return resulting timespec
 */
struct timespec timespec_add_ns(struct timespec ts, int64_t nsec);

/**
 * @brief get the system uptime as a timespec
 * @return uptime since boot
 */
struct timespec get_uptime(void);

/**
 * @brief get the current kernel time as a timespec
 * @return current time
 */
struct timespec get_ktime(void);

/**
 * @brief get the clock readout overhead in nanoseconds
 * @return readout overhead in nanoseconds
 */
uint32_t ktime_get_readout_overhead(void);


/**
 * @brief compute the difference between two timespecs in seconds
 * @param time1: later time
 * @param time0: earlier time
 * @return difference in seconds as a double
 */
double difftime(const struct timespec time1, const struct timespec time0);

/**
 * @brief compute the difference between two timespecs in nanoseconds
 * @param time1: later time
 * @param time0: earlier time
 * @return difference in nanoseconds as a double
 */
double difftime_ns(const struct timespec time1, const struct timespec time0);

/**
 * @brief initialize the kernel timekeeping subsystem
 * @param clock: the clocksource to use for timekeeping
 */
void time_init(struct clocksource *clock);

#endif /* _KERNEL_KTIME_H_ */
