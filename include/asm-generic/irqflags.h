/* SPDX-License-Identifier: GPL-2.0 */
/**
 * @file    include/asm-generic/irqflags.h
 * @ingroup interrupts
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
 * @brief generic wrapper for the architecture local-interrupt helpers
 *
 * Forwarding declarations of the local interrupt enable/disable/save/restore
 * helpers. The actual implementations are architecture-specific (SPARC:
 * arch/sparc/include/asm/irqflags.h) and operate on the processor's PSR PIL
 * field; these wrappers let common code disable/enable interrupts on the
 * current CPU without depending on architecture details.
 */

#ifndef _ASM_GENERIC_IRQFLAGS_H_
#define _ASM_GENERIC_IRQFLAGS_H_


#include <asm/irqflags.h>

/**
 * @brief enable interrupts on the local CPU
 */
void arch_local_irq_enable(void);

/**
 * @brief save the interrupt flags and disable interrupts
 * @return the saved interrupt flags
 */
unsigned long arch_local_irq_save(void);

/**
 * @brief restore previously saved interrupt flags
 * @param flags: flags previously returned by arch_local_irq_save
 */
void arch_local_irq_restore(unsigned long flags);

#endif /* _ASM_GENERIC_IRQFLAGS_H_ */
