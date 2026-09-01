/**
 * @file    include/kernel/watchdog.h
 * @ingroup timing
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @brief High-level watchdog policy and clockevent interface.
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

#ifndef _KERNEL_WATCHDOG_H_
#define _KERNEL_WATCHDOG_H_

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/clockevent.h>


/**
 * @brief watchdog timer operating modes
 */
enum watchdog_mode {
	WATCHDOG_UNLEASH,	/*!< disable watchdog (unleash) */
	WATCHDOG_LEASH		/*!< enable watchdog (leash) */
};

/**
 * @brief check if a clock event device is suitable for the watchdog
 * @param dev: the clock event device to evaluate
 */
void watchdog_check_device(struct clock_event_device *dev);

/**
 * @brief set the watchdog timer operating mode
 * @param mode: the desired watchdog mode (leash or unleash)
 * @return 0 on success, negative error code on failure
 */
int watchdog_set_mode(enum watchdog_mode mode);

/**
 * @brief set the watchdog timeout handler
 * @param handler: callback function invoked on watchdog timeout
 * @param userdata: opaque pointer passed to the handler
 */
void watchdog_set_handler(void (*handler)(void *), void *userdata);

/**
 * @brief feed (reset) the watchdog timer
 * @param nanoseconds: watchdog timeout in nanoseconds
 * @return 0 on success, negative error code on failure
 */
int watchdog_feed(unsigned long nanoseconds);


#endif /* _KERNEL_WATCHDOG_H_ */
