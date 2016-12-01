/**
 * @file   wrap_cpus_next_valid_pkt_size.h
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

#ifndef WRAP_CPUS_NEXT_VALID_PKT_SIZE_H
#define WRAP_CPUS_NEXT_VALID_PKT_SIZE_H

#include <shared.h>


/**
 * @brief tracks the functional status of the wrapper 
 */

static enum wrap_status cpus_next_valid_pkt_size_wrapper(enum wrap_status ws)
{
	static enum wrap_status status = DISABLED;

	if (ws != QUERY)
		status = ws;

	return status;
}

__attribute__((unused))
int32_t __real_cpus_next_valid_pkt_size(struct cpus_buffers *cpus);

/**
 * @brief a wrapper of cpus_next_valid_pkt_size
 */

__attribute__((unused))
int32_t __wrap_cpus_next_valid_pkt_size(struct cpus_buffers *cpus)
{
	if (cpus_next_valid_pkt_size_wrapper(QUERY) == DISABLED)
		return __real_cpus_next_valid_pkt_size(cpus);

	errno = 0xdead;
	return -1;
}


#endif
