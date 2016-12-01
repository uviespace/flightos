/**
 * @file   wrap_cbuf_read8.c
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
 * @brief a mockup of cbuf_peek16
 *
 */

#include <shared.h>
#include <stdint.h>
#include <wrap_cbuf_read8.h>


/**
 * @brief tracks the functional status of the wrapper 
 */

enum wrap_status cbuf_read8_wrapper(enum wrap_status ws)
{
	static enum wrap_status status = DISABLED;

	if (ws != QUERY)
		status = ws;

	return status;
}

/**
 * @brief a wrapper of cbuf_read8 depending on wrapper status
 */

uint32_t __wrap_cbuf_read8(struct circular_buffer8 *p_buf,
			    uint8_t *dest,
			    uint32_t elem)
{
	if (cbuf_read8_wrapper(QUERY) == DISABLED)
		return __real_cbuf_read8(p_buf, dest, elem);

	if (cbuf_read8_wrapper(QUERY) == SPECIAL)
		return 0;

	dest[0] = 0xde;

	return elem;
}
