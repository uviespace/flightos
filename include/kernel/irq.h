/**
 * @file    include/kernel/irq.h
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 * @date    December, 2013
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

#ifndef _KERNEL_IRQ_H_
#define _KERNEL_IRQ_H_

#include <kernel/types.h>


enum irqreturn {
        IRQ_NONE    = 0,
        IRQ_HANDLED = 1
};

typedef enum irqreturn irqreturn_t;

typedef irqreturn_t (*irq_handler_t)(unsigned int irq, void *);
enum isr_exec_priority {ISR_PRIORITY_NOW, ISR_PRIORITY_DEFERRED};


struct irq_data {
        unsigned int irq;
	enum isr_exec_priority priority;
	irq_handler_t handler;
	void *data;
};

struct irq_dev {
        unsigned int (*irq_enable)   (struct irq_data *data);
        void         (*irq_disable)  (struct irq_data *data);
        void         (*irq_mask)     (struct irq_data *data);
        void         (*irq_unmask)   (struct irq_data *data);
        void         (*irq_deferred) (void);
};



void irq_init(struct irq_dev *dev);

int irq_free(unsigned int irq, irq_handler_t handler, void *data);

int irq_request(unsigned int irq, enum isr_exec_priority priority,
		irq_handler_t handler, void *data);

int irq_exec_deferred(void);

#endif /* _KERNEL_IRQ_H_ */
