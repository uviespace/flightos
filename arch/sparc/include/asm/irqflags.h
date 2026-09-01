/**
 * @file    sparc/include/asm/irqflags.h
 * @ingroup interrupts
 *
 * @brief local interrupt enable/save/restore helpers
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

#ifndef _ARCH_SPARC_ASM_IRQFLAGS_H_
#define _ARCH_SPARC_ASM_IRQFLAGS_H_

/**
 * @brief enable local interrupts
 *
 * Clears the PSR PIL field so that all interrupt levels are enabled.
 */
void arch_local_irq_enable(void);

/**
 * @brief save the interrupt state and disable interrupts
 *
 * @return the previous interrupt state (PSR), usable with
 *	 arch_local_irq_restore()
 */
unsigned long arch_local_irq_save(void);

/**
 * @brief restore the previous local interrupt state
 *
 * @param flags the interrupt state to restore, previously obtained
 *	 from arch_local_irq_save()
 */
void arch_local_irq_restore(unsigned long flags);


/**
 * @brief disable local interrupts
 */
static inline void arch_local_irq_disable(void)
{
	arch_local_irq_save();
}


#endif /* _ARCH_SPARC_ASM_IRQFLAGS_H_ */
