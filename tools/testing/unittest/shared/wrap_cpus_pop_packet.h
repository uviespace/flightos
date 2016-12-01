/**
 * @file   wrap_cpus_pop_packet.h
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

#ifndef WRAP_CPUS_POP_PACKET
#define WRAP_CPUS_POP_PACKET

#include <shared.h>


/**
 * @brief tracks the functional status of the wrapper 
 */

static enum wrap_status cpus_pop_packet_wrapper(enum wrap_status ws)
{
	static enum wrap_status status = DISABLED;

	if (ws != QUERY)
		status = ws;

	return status;
}

__attribute__((unused))
int32_t __real_cpus_pop_packet(struct cpus_buffers *cpus, uint8_t *pus_pkt);


/**
 * @brief a wrapper of cpus_pop_packet
 */

__attribute__((unused))
int32_t __wrap_cpus_pop_packet(struct cpus_buffers *cpus, uint8_t *pus_pkt)
{
	if (cpus_pop_packet_wrapper(QUERY) == DISABLED)
		return __real_cpus_pop_packet(cpus, pus_pkt);

	errno = 0xdead;
	return -1;
}

#endif
