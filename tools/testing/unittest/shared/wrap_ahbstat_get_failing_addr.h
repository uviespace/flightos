/**
 * @file   wrap_ahbstat_get_failing_addr.h
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
 */

#ifndef WRAP_AHBSTAT_GET_FAILING_ADDR
#define WRAP_AHBSTAT_GET_FAILING_ADDR

#include <shared.h>
#include <stdint.h>
#include <ahb.h>

enum wrap_status ahbstat_get_failing_addr_wrapper(enum wrap_status ws);

uint32_t __real_ahbstat_get_failing_addr(void);

uint32_t __wrap_ahbstat_get_failing_addr(void);



#endif
