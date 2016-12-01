/**
 * @file   wrap_grspw2_add_pkt.c
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
#include <wrap_grspw2_add_pkt.h>


/**
 * @brief tracks the functional status of the wrapper 
 */

enum wrap_status grspw2_add_pkt_wrapper(enum wrap_status ws)
{
	static enum wrap_status status = DISABLED;

	if (ws != QUERY)
		status = ws;

	return status;
}


/**
 * @brief a wrapper of grspw2_add_pkt
 */

int32_t __wrap_grspw2_add_pkt(struct grspw2_core_cfg *cfg,
			const void *hdr,  uint32_t hdr_size,
			const void *data, uint32_t data_size)
{
	if (grspw2_add_pkt_wrapper(QUERY) == DISABLED)
		return __real_grspw2_add_pkt(cfg,
					     hdr, hdr_size,
					     data, data_size);

	return 1;
}
