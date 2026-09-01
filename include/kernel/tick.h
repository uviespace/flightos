/**
 * @file    include/kernel/tick.h
 * @ingroup timing
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @brief High-level tick-device selection and programming interface.
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

#ifndef _KERNEL_TICK_H_
#define _KERNEL_TICK_H_

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/clockevent.h>


/**
 * @brief tick timer operating modes
 */
enum tick_mode {
	TICK_MODE_PERIODIC,	/*!< periodic tick mode */
	TICK_MODE_ONESHOT	/*!< one-shot tick mode */
};

/**
 * @brief check if a clock event device is suitable for the tick
 * @param dev: the clock event device to evaluate
 */
void tick_check_device(struct clock_event_device *dev);

/**
 * @brief set the tick timer operating mode
 * @param mode: the desired tick mode (periodic or oneshot)
 * @return 0 on success, negative error code on failure
 */
int tick_set_mode(enum tick_mode mode);

/**
 * @brief schedule the next tick interrupt in nanoseconds from now
 * @param nanoseconds: time until next tick in nanoseconds
 * @return 0 on success, negative error code on failure
 */
int tick_set_next_ns(unsigned long nanoseconds);

/**
 * @brief schedule the next tick interrupt for a specific CPU
 * @param nanoseconds: time until next tick in nanoseconds
 * @param cpu: target CPU id
 * @return 0 on success, negative error code on failure
 */
int tick_set_next_ns_for_cpu(unsigned long nanoseconds, int cpu);

/**
 * @brief schedule the next tick interrupt at an absolute ktime
 * @param expires: absolute time of the next tick
 * @return 0 on success, negative error code on failure
 */
int tick_set_next_ktime(struct timespec expires);

/**
 * @brief get the minimum supported tick period in nanoseconds
 * @return minimum tick period in nanoseconds
 */
unsigned long tick_get_period_min_ns(void);

#endif /* _KERNEL_TICK_H_ */
