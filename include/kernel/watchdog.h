/**
 * @file    include/kernel/watchdog.h
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

#ifndef _KERNEL_WATCHDOG_H_
#define _KERNEL_WATCHDOG_H_

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/clockevent.h>


/* watchdog modes */
enum watchdog_mode {
	WATCHDOG_UNLEASH,
	WATCHDOG_LEASH
};



void watchdog_check_device(struct clock_event_device *dev);
int watchdog_set_mode(enum watchdog_mode mode);
void watchdog_set_handler(void (*handler)(void *), void *userdata);
int watchdog_feed(unsigned long nanoseconds);


#endif /* _KERNEL_WATCHDOG_H_ */
