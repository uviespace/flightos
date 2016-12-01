/**
 * @file   wrap_ahbstat_new_error.c
 * @ingroup mockups
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   2016
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
#include <wrap_ahbstat_new_error.h>


/**
 * @brief tracks the functional status of the wrapper
 */

enum wrap_status ahbstat_new_error_wrapper(enum wrap_status ws)
{
	static enum wrap_status status = DISABLED;

	if (ws != QUERY)
		status = ws;

	return status;
}


/**
 * @brief a wrapper of ahbstat_new_error
 */

uint32_t __wrap_ahbstat_new_error(void)
{
	if (ahbstat_new_error_wrapper(QUERY) == DISABLED)
		return __real_ahbstat_new_error();

	if (ahbstat_new_error_wrapper(QUERY) == SPECIAL)
		return 0;

	return 1;
}
