/**
 * @file   arch/sparc/include/irq.h
 * @ingroup timing
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
 */

#ifndef _SPARC_IRQ_H_
#define _SPARC_IRQ_H_


void leon_enable_irq(unsigned int irq, int cpu);
void leon_disable_irq(unsigned int irq, int cpu);
void leon_force_irq(unsigned int irq, int cpu);

#endif /* _SPARC_IRQ_H_ */

