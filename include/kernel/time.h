/**
 * @file    include/kernel/time.h
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

#ifndef _KERNEL_TIME_H_
#define _KERNEL_TIME_H_

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/clocksource.h>


#if 0
struct timespec {
	uint32_t tv_sec;	/* seconds */
	uint32_t tv_nsec;	/* nanoseconds */
};
#endif

/* we use the compiler-defined struct timespec at this time, but we can
 * at least verify the size of the types to see if we are compatible
 */
compile_time_assert((member_size(struct timespec, tv_sec)  == sizeof(uint32_t)),
		    TIMESPEC_SEC_SIZE_MISMATCH);
compile_time_assert((member_size(struct timespec, tv_nsec) == sizeof(uint32_t)),
		    TIMESPEC_NSEC_SIZE_MISMATCH);


struct timekeeper {
	struct clocksource *clock;
};



void time_get_uptime(struct timespec *ts);
void time_init(struct clocksource *clock);

#endif /* _KERNEL_TIME_H_ */
