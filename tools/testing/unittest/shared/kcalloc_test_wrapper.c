/**
 * @file   kcalloc_test_wrapper.c
 * @ingroup mockups
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   2017
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
 * @brief a mockup of kcalloc
 *
 */

#include <shared.h>
#include <stdint.h>
#include <stdlib.h>
#include <kcalloc_test_wrapper.h>

#define MALLOC_FAIL_AFTER 2

static uint32_t malloc_cnt;

/**
 * @brief tracks the functional status of the wrapper
 */

enum wrap_status kcalloc_test_wrapper(enum wrap_status ws)
{
	static enum wrap_status status = DISABLED;

	if (ws != QUERY)
		status = ws;

	if (ws == SPECIAL)
		malloc_cnt = 0;

	return status;
}

/**
 * @brief a wrapper of malloc depending on wrapper status
 *
 * @note since tests are done on the PC, instead of using __real_malloc(), we'll
 * just use libc's malloc()
 */

void *__wrap_kcalloc(size_t nmemb, size_t size)
{
	if (kcalloc_test_wrapper(QUERY) == DISABLED)
		return calloc(nmemb, size);

	if (kcalloc_test_wrapper(QUERY) == SPECIAL)
		if (malloc_cnt++ < MALLOC_FAIL_AFTER)
			return calloc(nmemb, size);

	return NULL;
}
