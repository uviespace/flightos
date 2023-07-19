/**
 * @file    include/kernel/edac.h
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @ingroup edac
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

#ifndef _KERNEL_EDAC_H_
#define _KERNEL_EDAC_H_

#include <kernel/types.h>



struct edac_dev {
        void           (*enable)            (void);
        void           (*disable)           (void);
        int            (*crit_seg_add)      (void *begin, void *end);
        int            (*crit_seg_rem)	    (void *begin, void *end);
	int            (*error_detected)    (void);
	unsigned long  (*get_error_addr)    (void);
	void           (*error_clear)       (void);
        void           (*inject_fault)      (void *addr, uint32_t mem_value, uint32_t edac_value);
	void           (*set_reset_handler) (void (*handler)(void *), void *data);
};


void edac_inject_fault(void *addr, uint32_t mem_value, uint32_t edac_value);

void edac_set_reset_callback(void (*handler)(void *), void *userdata);
int edac_critical_segment_add(void *begin, void *end);
int edac_critical_segment_rem(void *begin, void *end);

int edac_error_detected(void);
unsigned long edac_get_error_addr(void);
void edac_error_clear(void);

void edac_enable(void);
void edac_disable(void);

void edac_init(struct edac_dev *dev);

#endif /* _KERNEL_EDAC_H_ */
