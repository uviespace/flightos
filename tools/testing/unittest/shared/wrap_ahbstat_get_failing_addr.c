/**
 * @file   wrap_ahbstat_get_failing_addr.c
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
#include <wrap_ahbstat_get_failing_addr.h>
#include <iwf_fpga.h>
#include <wrap_malloc.h>

static uint32_t fail_cnt;

/**
 * @brief tracks the functional status of the wrapper
 */

enum wrap_status ahbstat_get_failing_addr_wrapper(enum wrap_status ws)
{
	static enum wrap_status status = DISABLED;

	if (ws != QUERY)
		status = ws;

	if (ws == SPECIAL)
		fail_cnt = 0;


	return status;
}


/**
 * @brief a wrapper of ahbstat_get_failing_addr
 */

uint32_t __wrap_ahbstat_get_failing_addr(void)
{
	if (ahbstat_get_failing_addr_wrapper(QUERY) == DISABLED)
		return __real_ahbstat_get_failing_addr();

	if (ahbstat_get_failing_addr_wrapper(QUERY) == SPECIAL) {

		switch (fail_cnt++) {
		case 0:
			return IWF_FPGA_FLASH_1_DATA_BASE;
		case 1:
			return IWF_FPGA_FLASH_2_DATA_BASE;
		case 2:
			return IWF_FPGA_FLASH_3_DATA_BASE;
		case 3:
			return IWF_FPGA_FLASH_4_DATA_BASE;
		default:
			return 0;
		}
	}

	return SRAM1_SW_ADDR;
}
