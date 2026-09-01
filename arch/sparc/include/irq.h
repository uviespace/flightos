/**
 * @file   arch/sparc/include/irq.h
 * @ingroup interrupts
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   July, 2016
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
 * @brief low-level LEON interrupt controller access
 *
 * Raw, per-CPU interrupt line masking helpers for the LEON interrupt
 * controllers. They operate directly on the controller registers and are used
 * by the LEON IRQ backend in arch/sparc/kernel/irq.c; the public, higher-level
 * API is the generic interface in kernel/irq.c (see include/kernel/irq.h).
 *
 * IRQ numbers map to controller bits as defined in asm/leon_reg.h: primary
 * interrupts 1-15 for LEON3/LEON4 and 0-15 for LEON2, extended (EIRQ)
 * interrupts 16-31 for LEON3/LEON4.
 */

#ifndef _SPARC_IRQ_H_
#define _SPARC_IRQ_H_


/**
 * @brief enable an interrupt
 *
 * @param irq the interrupt to enable
 * @param cpu the cpu for which the interrupt is to be enabled
 */
void leon_enable_irq(unsigned int irq, int cpu);

/**
 * @brief disable an interrupt
 *
 * @param irq the interrupt to disable
 * @param cpu the cpu for which the interrupt is to be disabled
 */
void leon_disable_irq(unsigned int irq, int cpu);

/**
 * @brief force an interrupt
 *
 * @param irq the interrupt to force
 * @param cpu the cpu on which to force the interrupt (set < 0 for all)
 *
 * @note interrupts must be enabled for this to work
 */
void leon_force_irq(unsigned int irq, int cpu);

#endif /* _SPARC_IRQ_H_ */
