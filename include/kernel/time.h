/**
 * @file    include/kernel/ktime.h
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
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


struct timekeeper {
	struct clocksource *clock;
	uint32_t readout_ns;	/* readout time overhead in ns */
};




ktime ktime_get(void);
ktime ktime_set(const unsigned long sec, const unsigned long nsec);
ktime timespec_to_ktime(struct timespec ts);

ktime ktime_add(const ktime t1, const ktime t2);
ktime ktime_sub(const ktime later, const ktime earlier);
ktime ktime_add_ns(const ktime t, const uint64_t nsec);
ktime ktime_sub_ns(const ktime t, const uint64_t nsec);
ktime ktime_add_us(const ktime t, const uint64_t usec);
ktime ktime_add_ms(const ktime t, const uint64_t msec);
ktime ktime_sub_us(const ktime t, const uint64_t usec);
ktime ktime_sub_ms(const ktime t, const uint64_t msec);
int ktime_compare(const ktime t1, const ktime t2);
bool ktime_after(const ktime t1, const ktime t2);
bool ktime_before(const ktime t1, const ktime t2);
int64_t ktime_delta(const ktime later, const ktime earlier);
int64_t ktime_us_delta(const ktime later, const ktime earlier);
int64_t ktime_ms_delta(const ktime later, const ktime earlier);
int64_t ktime_to_us(const ktime t);
int64_t ktime_to_ms(const ktime t);
ktime us_to_ktime(const int64_t usec);
ktime ms_to_ktime(const int64_t msec);


struct timespec timespec_add(struct timespec t1, struct timespec t2);
struct timespec ns_to_timespec(const int64_t nsec);
struct timespec timespec_add_ns(struct timespec ts, int64_t nsec);
struct timespec get_uptime(void);
struct timespec get_ktime(void);

uint32_t ktime_get_readout_overhead(void);


double difftime(const struct timespec time1, const struct timespec time0);
double difftime_ns(const struct timespec time1, const struct timespec time0);

void time_init(struct clocksource *clock);

#endif /* _KERNEL_KTIME_H_ */
