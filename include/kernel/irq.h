/**
 * @file    include/kernel/irq.h
 * @ingroup interrupts
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @ingroup irqsys
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
 * @brief declarations of the high-level, architecture-independent IRQ API
 *
 * This header defines the processor-independent IRQ interface and its data
 * structures. The interface is implemented in kernel/irq.c, which delegates
 * all hardware interaction to a single platform backend (struct irq_dev)
 * registered via irq_init(); the only current backend is the LEON one in
 * arch/sparc/kernel/irq.c.
 *
 * Key types:
 *   - enum irqreturn         : handler return convention (IRQ_NONE /
 *                              IRQ_HANDLED). Note: the LEON deferred-handler
 *                              path currently keys the re-queue decision on
 *                              this return value (see irq_flow in
 *                              arch/sparc/kernel/irq.c, needs review).
 *   - enum isr_exec_priority : immediate (ISR_PRIORITY_NOW) or deferred
 *                              (ISR_PRIORITY_DEFERRED) execution.
 *   - struct irq_data        : per-request parameters handed to the backend
 *                              operations (IRQ number, priority, handler,
 *                              opaque data pointer).
 *   - struct irq_dev         : the callback table a platform backend must
 *                              implement to plug into the generic layer
 *                              (irq enable/disable/mask/unmask, deferred
 *                              execution, affinity, level).
 *
 * The generic layer is designed to hold a single irq_ctrl (one backend);
 * support for multiple backends is not present.
 */

#ifndef _KERNEL_IRQ_H_
#define _KERNEL_IRQ_H_

#include <kernel/types.h>


/**
 * @brief IRQ handler return values
 */
enum irqreturn {
        IRQ_NONE    = 0,	/*!< IRQ was not handled */
        IRQ_HANDLED = 1		/*!< IRQ was handled successfully */
};

/** @brief IRQ handler return type */
typedef enum irqreturn irqreturn_t;

/** @brief IRQ handler function pointer type */
typedef irqreturn_t (*irq_handler_t)(unsigned int irq, void *);

/**
 * @brief ISR execution priority levels
 */
enum isr_exec_priority {ISR_PRIORITY_NOW, ISR_PRIORITY_DEFERRED};

/**
 * @brief per-IRQ data passed to handlers
 */
struct irq_data {
        unsigned int irq;			/*!< IRQ number */
	enum isr_exec_priority priority;	/*!< execution priority */
	irq_handler_t handler;			/*!< handler function */
	void *data;				/*!< private data for handler */
};

/**
 * @brief IRQ device operations structure
 */
struct irq_dev {
        unsigned int (*irq_enable)        (struct irq_data *data);  /*!< enable the IRQ */
        void         (*irq_disable)       (struct irq_data *data);  /*!< disable the IRQ */
        void         (*irq_mask)          (struct irq_data *data);  /*!< mask the IRQ */
        void         (*irq_unmask)        (struct irq_data *data);  /*!< unmask the IRQ */
        void         (*irq_deferred)      (void);                   /*!< deferred IRQ handler */
        void         (*irq_set_affinity)  (unsigned int irq, int cpu); /*!< set CPU affinity */
        void         (*irq_set_level)	  (unsigned int irq, unsigned int priority); /*!< set IRQ priority level */
};


/**
 * @brief initialize the IRQ subsystem with a device driver
 * @param dev: the IRQ device operations structure
 */
void irq_init(struct irq_dev *dev);

/**
 * @brief free a previously requested IRQ
 * @param irq: the IRQ number to free
 * @param handler: the handler that was registered
 * @param data: the private data that was passed to irq_request
 * @return 0 on success, negative error code on failure
 */
int irq_free(unsigned int irq, irq_handler_t handler, void *data);

/**
 * @brief request an IRQ line
 * @param irq: the IRQ number to request
 * @param priority: ISR execution priority (NOW or DEFERRED)
 * @param handler: handler function to call on interrupt
 * @param data: opaque pointer passed to handler
 * @return 0 on success, negative error code on failure
 */
int irq_request(unsigned int irq, enum isr_exec_priority priority,
		irq_handler_t handler, void *data);

/**
 * @brief execute any pending deferred IRQ handlers
 * @return 0 on success, negative error code on failure
 */
int irq_exec_deferred(void);

/**
 * @brief set the CPU affinity for an IRQ
 * @param irq: the IRQ number
 * @param cpu: target CPU id
 * @return 0 on success, negative error code on failure
 */
int irq_set_affinity(unsigned int irq, int cpu);

/**
 * @brief set the priority level for an IRQ
 * @param irq: the IRQ number
 * @param priority: the priority level
 */
void irq_set_level(unsigned int irq, unsigned int priority);

#endif /* _KERNEL_IRQ_H_ */
