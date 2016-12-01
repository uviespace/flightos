/**
 * @file   wrap_fpga_flash_empty.c
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
#include <wrap_fpga_flash_empty.h>


/**
 * @brief tracks the functional status of the wrapper 
 */

enum wrap_status fpga_flash_empty_wrapper(enum wrap_status ws)
{
	static enum wrap_status status = DISABLED;

	if (ws != QUERY)
		status = ws;

	return status;
}


/**
 * @brief a wrapper of fpga_flash_empty
 */

int32_t __wrap_fpga_flash_empty(uint32_t unit, uint32_t block)
{
	if (fpga_flash_empty_wrapper(QUERY) == DISABLED)
		return __real_fpga_flash_empty(unit, block);

	return 1;
}
