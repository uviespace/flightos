/* SPDX-License-Identifier: GPL-2.0 */
/**
 * @file    include/asm-generic/irqflags.h
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
 */

#ifndef _ASM_GENERIC_IRQFLAGS_H_
#define _ASM_GENERIC_IRQFLAGS_H_


void arch_local_irq_enable(void);
unsigned long arch_local_irq_save(void);
void arch_local_irq_restore(unsigned long flags);

#endif /* _ASM_GENERIC_IRQFLAGS_H_ */
