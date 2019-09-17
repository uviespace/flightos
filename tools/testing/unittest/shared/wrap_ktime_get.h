/**
 * @file   wrap_ktime_get.h
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
 */

#ifndef WRAP_KTIME_GET
#define WRAP_KTIME_GET

#include <shared.h>
#include <stdint.h>
#include <kernel/time.h>

enum wrap_status ktime_get_wrapper(enum wrap_status ws);

ktime __real_ktime_get(void);

ktime __wrap_ktime_get(void);

void ktime_wrap_set_time(ktime time);



#endif
