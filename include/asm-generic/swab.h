/**
 * @file    include/asm-generic/swab.h
 *
 * @copyright GPLv2
 *
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

#ifndef _ASM_GENERIC_SWAB_H_
#define _ASM_GENERIC_SWAB_H_

#include <kernel/types.h>

/** @brief byte-swap a 16-bit value */
#define swab16(x) ((uint16_t)(					\
	(((uint16_t)(x) & (uint16_t)0x00ffU) << 8) |		\
	(((uint16_t)(x) & (uint16_t)0xff00U) >> 8)))

/** @brief byte-swap a 32-bit value */
#define swab32(x) ((uint32_t)(					\
	(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |	\
	(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |	\
	(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |	\
	(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24)))


/** @brief swap the halfwords of a 32-bit value */
#define swahw32(x) ((uint32_t)(					\
	(((uint32_t)(x) & (uint32_t)0x0000ffffUL) << 16) |	\
	(((uint32_t)(x) & (uint32_t)0xffff0000UL) >> 16)))

/** @brief swap the bytes within each halfword of a 32-bit value */
#define swahb32(x) ((uint32_t)(					\
	(((uint32_t)(x) & (uint32_t)0x00ff00ffUL) << 8) |	\
	(((uint32_t)(x) & (uint32_t)0xff00ff00UL) >> 8)))

/** @brief swap the nibbles within each byte of a 32-bit value */
#define swabb32(x) ((uint32_t)(					\
	(((uint32_t)(x) & (uint32_t)0x0f0f0f0fUL) << 4) |	\
	(((uint32_t)(x) & (uint32_t)0xf0f0f0f0UL) >> 4)))

#endif /* _ASM_GENERIC_SWAB_H_ */
