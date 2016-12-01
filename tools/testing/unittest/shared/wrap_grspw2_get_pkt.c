/**
 * @file   wrap_grspw2_get_pkt.c
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
#include <wrap_grspw2_get_pkt.h>


/**
 * @brief tracks the functional status of the wrapper 
 */

enum wrap_status grspw2_get_pkt_wrapper(enum wrap_status ws)
{
	static enum wrap_status status = DISABLED;

	if (ws != QUERY)
		status = ws;

	return status;
}


/**
 * @brief a wrapper of grspw2_get_pkt
 */

uint32_t __wrap_grspw2_get_pkt(struct grspw2_core_cfg *cfg, uint8_t *pkt)
{
	if (grspw2_get_pkt_wrapper(QUERY) == DISABLED)
		return __real_grspw2_get_pkt(cfg, pkt);

	return 0;
}
