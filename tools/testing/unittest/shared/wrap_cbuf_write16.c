/**
 * @file   wrap_cbuf_write16.c
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
 * @brief a mockup of cbuf_write16
 *
 */

#include <shared.h>
#include <stdint.h>
#include <wrap_cbuf_write16.h>


/**
 * @brief tracks the functional status of the wrapper 
 */

enum wrap_status cbuf_write16_wrapper(enum wrap_status ws)
{
	static enum wrap_status status = DISABLED;

	if (ws != QUERY)
		status = ws;

	return status;
}


/**
 * @brief a wrapper of cbuf_write16 depending on wrapper status
 */

uint32_t __wrap_cbuf_write16(struct circular_buffer16 *p_buf,
			     uint16_t *src,
			     uint32_t elem)
{
	if (cbuf_write16_wrapper(QUERY) == DISABLED)
		return __real_cbuf_write16(p_buf, src, elem);

	return 0;
}
