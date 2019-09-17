/**
 * @file   wrap_ktime_get.c
 * @ingroup mockups
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   2015
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
 */

#include <shared.h>
#include <stdint.h>
#include <kernel/time.h>
#include <wrap_ktime_get.h>


static ktime kernel_time;


/**
 * @brief sets the internal kernel time
 */

void ktime_wrap_set_time(ktime time)
{
	kernel_time = time;
}

/**
 * @brief tracks the functional status of the wrapper
 */

enum wrap_status ktime_get_wrapper(enum wrap_status ws)
{
	static enum wrap_status status = DISABLED;

	if (ws != QUERY)
		status = ws;

	return status;
}


/**
 * @brief a wrapper of ktime_get
 */

ktime __wrap_ktime_get(void)
{
	if (ktime_get_wrapper(QUERY) == DISABLED)
		return __real_ktime_get();

	return kernel_time;
}
