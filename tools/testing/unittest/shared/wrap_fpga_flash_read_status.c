/**
 * @file   wrap_fpga_flash_read_status.c
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
#include <wrap_fpga_flash_read_status.h>

static uint32_t cnt;

/**
 * @brief tracks the functional status of the wrapper 
 */

enum wrap_status fpga_flash_read_status_wrapper(enum wrap_status ws)
{
	static enum wrap_status status = DISABLED;

	if (ws != QUERY) {
		status = ws;
		cnt = 0;
	}


	return status;
}


/**
 * @brief a wrapper of fpga_flash_read_status
 */

int32_t __wrap_fpga_flash_read_status(uint32_t unit, uint32_t block)
{
	static uint32_t status;


	if (fpga_flash_read_status_wrapper(QUERY) == DISABLED)
		return __real_fpga_flash_read_status(unit, block);
	
	if (fpga_flash_read_status_wrapper(QUERY) == SPECIAL)
		status = FLASH_STATUS_FAIL;
	else
		status = 0;

	cnt++;
	
	/* be careful changing these, the particular order is
	 * needed in flash_erase_block_test()
	 */ 

	switch (cnt) {
	case 1:
	case 2:
		return FLASH_BLOCK_VALID;
	case 3:
		return status;
		break;

	case 4:
		fpga_flash_read_status_wrapper(DISABLED);
		cnt = 0;
		return 0;
		break;
	default:
		return cnt;
	}
}
