/**
 * @file    include/kernel/memscrub.h
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @ingroup kmem
 * @ingroup memscrub
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

#ifndef _KERNEL_MEMSCRUB_H_
#define _KERNEL_MEMSCRUB_H_

/**
 * @brief remove a memory segment from scrubbing
 * @param begin: start address of the segment
 * @param end: end address of the segment (exclusive)
 * @return 0 on success, negative error code on failure
 */
int memscrub_seg_rem(unsigned long begin, unsigned long end);

/**
 * @brief add a memory segment for scrubbing
 * @param begin: start address of the segment
 * @param end: end address of the segment (exclusive)
 * @param wpc: words per cycle for the scrub operation
 * @return 0 on success, negative error code on failure
 */
int memscrub_seg_add(unsigned long begin, unsigned long end, unsigned short wpc);

#endif /* _KERNEL_MEMSCRUB_H_ */
