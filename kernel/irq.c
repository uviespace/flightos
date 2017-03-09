/**
 * @file kernel/irq.c
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * this implements the high-level IRQ logic (which is admittedly not very
 * thought-through)
 */


#include <errno.h>
#include <kernel/irq.h>
#include <kernel/export.h>

static struct irq_dev *irq_ctrl;


/**
 * @brief request a slot for an interrupt handler
 */

int irq_request(unsigned int irq, enum isr_exec_priority priority,
		irq_handler_t handler, void *data)
{
	struct irq_data cfg;


	if (!irq_ctrl)
		return -EINVAL;

	if (!irq_ctrl->irq_enable)
		return -EINVAL;

	cfg.irq      = irq;
	cfg.priority = priority;
	cfg.handler  = handler;
	cfg.data     = data;

	return irq_ctrl->irq_enable(&cfg);
}
EXPORT_SYMBOL(irq_request);

/**
 * @brief release an interrupt handler
 */

int irq_free(unsigned int irq, irq_handler_t handler, void *data)
{
	struct irq_data cfg;


	if (!irq_ctrl)
		return -EINVAL;

	if (!irq_ctrl->irq_disable)
		return -EINVAL;

	cfg.irq      = irq;
	cfg.handler  = handler;
	cfg.data     = data;

	irq_ctrl->irq_disable(&cfg);

	return 0;
}
EXPORT_SYMBOL(irq_free);


/**
 * @brief execute deferred interrupt handlers
 */

int irq_exec_deferred(void)
{
	if (!irq_ctrl)
		return -EINVAL;

	if (irq_ctrl->irq_deferred)
		irq_ctrl->irq_deferred();

	return 0;
}


/**
 * @brief initialise the IRQ system
 */

void irq_init(struct irq_dev *dev)
{
	irq_ctrl = dev;
}
