/**
 * @file   arch/sparc/include/grtimer_longcount.h
 * @ingroup timing
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   July, 2016
 *
 * @copyright GPLv2
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @brief Implements a long-counting (uptime) clock using the LEON3 GRTIMER
 *
 */

#ifndef _SPARC__GRTIMER_LONGCOUNT_H_
#define _SPARC__GRTIMER_LONGCOUNT_H_

#include <grtimer.h>

/**
 * "coarse" contains the counter of the secondary (chained) timer in
 * multiples of seconds and is chained
 * to the "fine" timer, which should hence underflow in a 1-second cycle
 */

struct grtimer_uptime {
	uint32_t coarse;
	uint32_t fine;
};


/**
 * @brief start the long-counting uptime timer using two chained grtimers
 * @param rtu a struct grtimer_unit
 * @param scaler_reload the scaler reload value
 * @param fine_ticks_per_sec a timer reload value in ticks per second
 * @param coarse_ticks_max a timer reload value in ticks per second
 *
 * @return -1 if fine_ticks_per_sec is not an integer multiple of
 *         scaler_reload, 0 otherwise
 */
int32_t grtimer_longcount_start(struct grtimer_unit *rtu,
				uint32_t scaler_reload,
				uint32_t fine_ticks_per_sec,
				uint32_t coarse_ticks_max);

/**
 * @brief get the uptime since the long-counting timer was started
 * @param rtu a struct grtimer_unit
 * @param up a struct grtimer_uptime to store the result
 */
void grtimer_longcount_get_uptime(struct grtimer_unit *rtu,
				  struct grtimer_uptime *up);

/**
 * @brief compute the time difference between two uptime timestamps
 * @param rtu a struct grtimer_unit
 * @param time1 a struct grtimer_uptime (later timestamp)
 * @param time0 a struct grtimer_uptime (earlier timestamp)
 *
 * @return time difference in seconds represented as double
 */
double grtimer_longcount_difftime(struct grtimer_unit *rtu,
				  struct grtimer_uptime time1,
				  struct grtimer_uptime time0);

/**
 * @brief get the time elapsed since the last latch in CPU cycles
 * @param rtu a struct grtimer_unit
 *
 * @return time in CPU cycles since the last latch event
 */
uint32_t grtimer_longcount_get_latch_time_diff(struct grtimer_unit *rtu);

#endif /* _SPARC_GRTIMER_LONGCOUNT_H_ */
