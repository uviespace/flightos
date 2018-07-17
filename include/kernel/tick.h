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

#ifndef _KERNEL_TICK_H_
#define _KERNEL_TICK_H_

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/clockevent.h>


/* tick modes */
enum tick_mode {
	TICK_MODE_PERIODIC,
	TICK_MODE_ONESHOT
};


void tick_check_device(struct clock_event_device *dev);
int tick_set_mode(enum tick_mode mode);
int tick_set_next_ns(unsigned long nanoseconds);
int tick_set_next_ktime(struct timespec expires);

#endif /* _KERNEL_TICK_H_ */
