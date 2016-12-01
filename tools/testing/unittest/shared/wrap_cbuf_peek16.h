/**
 * @file   wrap_cbuf_peek16.h
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

#ifndef WRAP_CBUF_PEEK16
#define WRAP_CBUF_PEEK16

#include <shared.h>
#include <circular_buffer16.h>

enum wrap_status cbuf_peek16_wrapper(enum wrap_status ws);

uint32_t __real_cbuf_peek16(struct circular_buffer16 *p_buf,
			    uint16_t *dest,
			    uint32_t elem);

uint32_t __wrap_cbuf_peek16(struct circular_buffer16 *p_buf,
			    uint16_t *dest,
			    uint32_t elem);



#endif
