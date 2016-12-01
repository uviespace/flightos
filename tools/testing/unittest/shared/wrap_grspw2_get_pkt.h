/**
 * @file   wrap_grspw2_get_pkt.h
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
 */

#ifndef WRAP_GRSPW2_GET_PKT
#define WRAP_GRSPW2_GET_PKT

#include <shared.h>
#include <stdint.h>
#include <grspw2.h>

enum wrap_status grspw2_get_pkt_wrapper(enum wrap_status ws);

uint32_t __real_grspw2_get_pkt(struct grspw2_core_cfg *cfg, uint8_t *pkt);
uint32_t __wrap_grspw2_get_pkt(struct grspw2_core_cfg *cfg, uint8_t *pkt);



#endif
