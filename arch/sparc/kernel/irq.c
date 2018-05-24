/**
 * @file arch/sparc/kernel/irq.c
 * 
 * @ingroup sparc
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
 * @todo eventually replace catch_interrupt() libgloss/newlib functionality
 *       with local/custom code and rework the globals
 * @todo irq configuration should only be done through a syscall,
 *       so traps are disabled
 *
 * @brief an IRQ manager that dispatches interrupts to registered
 *	  handler functions
 *
 * Implements an interrupt handler that supports registration of a predefined
 * arbitrary number of handler function per interrupt with immediate or deferred
 * execution priorities. Callbacks are tracked in linked lists that  should
 * always be allocated in a contiguous block of memory for proper cache
 * hit rate.
 *
 * @image html sparc/sparc_irq_manager.png "IRQ manager"
 *
 * @note this implements the processor specific IRQ logic
 *
 * @todo once we have fully functional AMBA bus scan, we might want to rework
 *	 the static interrupt assignments...
 *
 *
 * @note We do NOT cache-bypass read from interrupt control registers when
 *	 handling interrupts. It should not be necessary and can
 *	 potentially save a few cycles.
 */




#include <kernel/irq.h>
#include <kernel/kmem.h>
#include <kernel/printk.h>
#include <kernel/kernel.h>

#include <errno.h>
#include <list.h>

#include <asm/io.h>
#include <asm/leon.h>
#include <asm/leon_reg.h>
#include <asm/spinlock.h>
#include <asm/irq.h>


struct irl_vector_elem {
	irq_handler_t handler;
	enum isr_exec_priority priority;
	void *data;
	unsigned int irq;
	struct list_head handler_node;
};


/* maximum number of interrupt handlers we will allow */
#define IRL_POOL_SIZE	64
/* the maximum queue size for deferred handlers  */
#define IRL_QUEUE_SIZE	64



#ifdef CONFIG_LEON2
#define IRL_SIZE	LEON2_IRL_SIZE
#define EIRL_SIZE	LEON2_EIRL_SIZE

static struct leon2_irqctrl_registermap  *leon_irqctrl_regs;
static struct leon2_eirqctrl_registermap *leon_eirqctrl_regs;

#endif /* CONFIG_LEON2 */

#ifdef CONFIG_LEON3

#define IRL_SIZE	LEON3_IRL_SIZE
#define EIRL_SIZE	LEON3_IRL_SIZE

static struct leon3_irqctrl_registermap *leon_irqctrl_regs;

#endif /* CONFIG_LEON3 */


static struct list_head	irl_pool_head;
static struct list_head	irl_queue_head;
static struct list_head	irq_queue_pool_head;

static struct list_head	*irl_vector;
static struct list_head	*eirl_vector;

static struct irl_vector_elem *irl_pool;
static struct irl_vector_elem *irl_queue_pool;


static unsigned int leon_eirq;


#ifdef CONFIG_IRQ_STATS_COLLECT

#include <kernel/string.h>
#include <kernel/sysctl.h>

static struct {
	uint32_t irl;
	uint32_t eirl;
	uint32_t irl_irq[IRL_SIZE];
	uint32_t eirl_irq[EIRL_SIZE];
} irqstat;


static ssize_t irl_show(__attribute__((unused)) struct sysobj *sobj,
			__attribute__((unused)) struct sobj_attribute *sattr,
			char *buf)
{
	uint32_t irq;


	if (!strcmp("irl", sattr->name))
		return sprintf(buf, "%lu", irqstat.irl);

	irq = atoi(sattr->name);

	if (irq > IRL_SIZE)
		return 0;

	return sprintf(buf, "%lu", irqstat.irl_irq[irq]);
}

static ssize_t irl_store(__attribute__((unused)) struct sysobj *sobj,
			 __attribute__((unused)) struct sobj_attribute *sattr,
			 __attribute__((unused)) const char *buf,
			 __attribute__((unused)) size_t len)
{
	uint32_t irq;


	if (!strcmp("irl", sattr->name)) {
		irqstat.irl = 0;
		return 0;
	}

	irq = atoi(sattr->name);

	if (irq > IRL_SIZE)
		return 0;

	irqstat.irl_irq[irq] = 0;

	return 0;
}

static ssize_t eirl_show(__attribute__((unused)) struct sysobj *sobj,
			 __attribute__((unused)) struct sobj_attribute *sattr,
			 char *buf)
{
	uint32_t irq;


	if (!strcmp("eirl", sattr->name))
		return sprintf(buf, "%lu", irqstat.eirl);

	irq = atoi(sattr->name);

	if (irq > EIRL_SIZE)
		return 0;

	return sprintf(buf, "%lu", irqstat.eirl_irq[irq]);
}

static ssize_t eirl_store(__attribute__((unused)) struct sysobj *sobj,
			  __attribute__((unused)) struct sobj_attribute *sattr,
			  __attribute__((unused)) const char *buf,
			  __attribute__((unused)) size_t len)
{
	uint32_t irq;


	if (!strcmp("eirl", sattr->name)) {
		irqstat.eirl = 0;
		return 0;
	}

	irq = atoi(sattr->name);

	if (irq > EIRL_SIZE)
		return 0;

	irqstat.eirl_irq[irq] = 0;

	return 0;
}

__extension__
static struct sobj_attribute irl_attr[] = {
	__ATTR(irl,  irl_show, irl_store),
	__ATTR(0,  irl_show, irl_store), __ATTR(1,  irl_show, irl_store),
	__ATTR(2,  irl_show, irl_store), __ATTR(3,  irl_show, irl_store),
	__ATTR(4,  irl_show, irl_store), __ATTR(5,  irl_show, irl_store),
	__ATTR(6,  irl_show, irl_store), __ATTR(7,  irl_show, irl_store),
	__ATTR(8,  irl_show, irl_store), __ATTR(9, irl_show, irl_store),
	__ATTR(10, irl_show, irl_store), __ATTR(11, irl_show, irl_store),
	__ATTR(12, irl_show, irl_store), __ATTR(13, irl_show, irl_store),
	__ATTR(14, irl_show, irl_store), __ATTR(15, irl_show, irl_store),
	};

static struct sobj_attribute *irl_attributes[] = {
	&irl_attr[0],  &irl_attr[1],  &irl_attr[2],  &irl_attr[3],
	&irl_attr[4],  &irl_attr[5],  &irl_attr[6],  &irl_attr[7],
	&irl_attr[8],  &irl_attr[9],  &irl_attr[10], &irl_attr[11],
	&irl_attr[12], &irl_attr[13], &irl_attr[14], &irl_attr[15],
	&irl_attr[16], NULL};

__extension__
static struct sobj_attribute eirl_attr[] = {
	__ATTR(eirl,  eirl_show, eirl_store),
	__ATTR(0,  eirl_show, eirl_store), __ATTR(1,  eirl_show, eirl_store),
	__ATTR(2,  eirl_show, eirl_store), __ATTR(3,  eirl_show, eirl_store),
	__ATTR(4,  eirl_show, eirl_store), __ATTR(5,  eirl_show, eirl_store),
	__ATTR(6,  eirl_show, eirl_store), __ATTR(7,  eirl_show, eirl_store),
	__ATTR(8,  eirl_show, eirl_store), __ATTR(9, eirl_show, eirl_store),
	__ATTR(10, eirl_show, eirl_store), __ATTR(11, eirl_show, eirl_store),
	__ATTR(12, eirl_show, eirl_store), __ATTR(13, eirl_show, eirl_store),
	__ATTR(14, eirl_show, eirl_store), __ATTR(15,  eirl_show, eirl_store),
#ifdef CONFIG_LEON2
	__ATTR(16, eirl_show, eirl_store), __ATTR(17, eirl_show, eirl_store),
	__ATTR(18, eirl_show, eirl_store), __ATTR(19, eirl_show, eirl_store),
	__ATTR(20, eirl_show, eirl_store), __ATTR(21, eirl_show, eirl_store),
	__ATTR(22, eirl_show, eirl_store), __ATTR(23, eirl_show, eirl_store),
	__ATTR(24, eirl_show, eirl_store), __ATTR(25, eirl_show, eirl_store),
	__ATTR(26, eirl_show, eirl_store), __ATTR(27, eirl_show, eirl_store),
	__ATTR(28, eirl_show, eirl_store), __ATTR(29, eirl_show, eirl_store),
	__ATTR(30, eirl_show, eirl_store), __ATTR(31, eirl_show, eirl_store),
#endif /* CONFIG_LEON2 */
};

__extension__
static struct sobj_attribute *eirl_attributes[] = {
	&eirl_attr[0],  &eirl_attr[1],  &eirl_attr[2],  &eirl_attr[3],
	&eirl_attr[4],  &eirl_attr[5],  &eirl_attr[6],  &eirl_attr[7],
	&eirl_attr[8],  &eirl_attr[9],  &eirl_attr[10], &eirl_attr[11],
	&eirl_attr[12], &eirl_attr[13], &eirl_attr[14], &eirl_attr[15],
	&eirl_attr[16],

#ifdef CONFIG_LEON2
	&eirl_attr[17], &eirl_attr[18], &eirl_attr[19], &eirl_attr[20],
	&eirl_attr[21], &eirl_attr[22], &eirl_attr[23], &eirl_attr[24],
	&eirl_attr[25], &eirl_attr[26], &eirl_attr[27], &eirl_attr[28],
	&eirl_attr[29], &eirl_attr[30], &eirl_attr[31], &eirl_attr[32],
#endif /* CONFIG_LEON2 */
	NULL};

#endif /* CONFIG_IRQ_STATS_COLLECT */


#ifndef CONFIG_ARCH_CUSTOM_BOOT_CODE
/* provided by BCC's libgloss  */
extern int catch_interrupt (int func, int irq);
#endif /* CONFIG_ARCH_CUSTOM_BOOT_CODE */


/**
 * @brief clear (acknowledge) a pending IRQ
 *
 * @param irq the interrupt to clear
 */

static void leon_clear_irq(unsigned int irq)
{
#ifdef CONFIG_LEON3
	iowrite32be((1 << irq), &leon_irqctrl_regs->irq_clear);
#endif /* CONFIG_LEON3 */

#ifdef CONFIG_LEON2
	if (irq < IRL_SIZE)
		iowrite32be((1 << irq), &leon_irqctrl_regs->irq_clear);
	else if (irq < (IRL_SIZE + EIRL_SIZE))
#ifdef CONFIG_MPPB /* what what??? O.o */
		iowrite32be((1 << LEON_REAL_EIRQ(irq)),
			    &leon_eirqctrl_regs->eirq_status);
#else
	iowrite32be((1 << LEON_REAL_EIRQ(irq)),
			    &leon_eirqctrl_regs->eirq_clear);
#endif
#endif /* CONFIG_LEON2 */
}


/**
 * @brief unmask an IRQ
 *
 * @param irq the interrupt to unmask
 * @param cpu the cpu for which the interrupt is to be unmasked
 */

static void leon_unmask_irq(unsigned int irq, unsigned int cpu)
{
	uint32_t mask;


#ifdef CONFIG_LEON3
	mask = ioread32be(&leon_irqctrl_regs->irq_mpmask[cpu]);
	mask |= (1 << irq);
	iowrite32be(mask, &leon_irqctrl_regs->irq_mpmask[cpu]);
#endif /* CONFIG_LEON3 */

#ifdef CONFIG_LEON2
	if (irq < IRL_SIZE) {
		mask = ioread32be(&leon_irqctrl_regs->irq_mask);
		mask |= (1 << irq);
		iowrite32be(mask, &leon_irqctrl_regs->irq_mask);
	} else if (irq < (IRL_SIZE + EIRL_SIZE)) {
		mask = ioread32be(&leon_eirqctrl_regs->eirq_mask);
		mask |= (1 << LEON_REAL_EIRQ(irq));
		iowrite32be(mask, &leon_eirqctrl_regs->eirq_mask);
	}
#endif /* CONFIG_LEON2 */
}


/**
 * @brief mask an IRQ
 *
 * @param irq the interrupt to mask
 * @param cpu the cpu for which the interrupt is to be masked
 */

static void leon_mask_irq(unsigned int irq, unsigned int cpu)
{
	uint32_t mask;


#ifdef CONFIG_LEON3
	mask = ioread32be(&leon_irqctrl_regs->irq_mpmask[cpu]);

	mask &= ~(1 << irq);

	iowrite32be(mask, &leon_irqctrl_regs->irq_mpmask[cpu]);
#endif /* CONFIG_LEON3 */

#ifdef CONFIG_LEON2
	if (irq < IRL_SIZE) {
		mask = ioread32be(&leon_irqctrl_regs->irq_mask);
		mask &= ~(1 << irq);
		iowrite32be(mask, &leon_irqctrl_regs->irq_mask);
	} else if (irq < (IRL_SIZE + EIRL_SIZE)) {
		mask = ioread32be(&leon_eirqctrl_regs->eirq_mask);
		mask &= ~(1 << LEON_REAL_EIRQ(irq));
		iowrite32be(mask, &leon_eirqctrl_regs->eirq_mask);
	}
#endif /* CONFIG_LEON2 */
}

/**
 * @brief enable an interrupt
 *
 * @param irq the interrupt to enable
 * @param cpu the cpu for which the interrupt is to be enabled
 */

static void leon_enable_irq(unsigned int irq, unsigned int cpu)
{
	leon_clear_irq(irq);

	leon_unmask_irq(irq, cpu);
}


/**
 * @brief disable an interrupt
 *
 * @param irq the interrupt to disable
 * @param cpu the cpu for which the interrupt is to be disabled
 */

static void leon_disable_irq(unsigned int irq, unsigned int cpu)
{
	leon_clear_irq(irq);

	leon_mask_irq(irq, cpu);
}


/**
 * @brief queue a handler for delayed exectuion
 *
 * @param p_elem	a pointer to a struct IRQVectorElem
 *
 * @returns 0 on success
 */

static int leon_irq_queue(struct irl_vector_elem *p_elem)
{
	uint32_t psr_flags;
	struct irl_vector_elem *p_queue;


	if (unlikely(list_empty(&irq_queue_pool_head)))
		return -EBUSY;


	psr_flags = spin_lock_save_irq();

	p_queue = list_entry((&irq_queue_pool_head)->next,
			     struct irl_vector_elem, handler_node);

	p_queue->handler  = p_elem->handler;
	p_queue->priority = p_elem->priority;
	p_queue->data     = p_elem->data;
	p_queue->irq      = p_elem->irq;

	list_move_tail(&p_queue->handler_node, &irl_queue_head);

	spin_lock_restore_irq(psr_flags);

	return 0;
}


/**
 * @brief execute deferred (low-priority) handlers
 */

void leon_irq_queue_execute(void)
{
	struct irl_vector_elem *p_elem = NULL;
	struct irl_vector_elem *p_tmp;


	if (list_empty(&irl_queue_head))
		return;

	list_for_each_entry_safe(p_elem, p_tmp,
				 &irl_queue_head, handler_node) {

		list_del(&p_elem->handler_node);

		if (likely(p_elem->handler)) {

			if (!p_elem->handler(p_elem->irq, p_elem->data))
				leon_irq_queue(p_elem);
			else
				list_add_tail(&p_elem->handler_node,
					      &irq_queue_pool_head);

		} else {
			list_add_tail(&p_elem->handler_node,
				      &irq_queue_pool_head);
		}
	}
}


/**
 * @brief he central interrupt handling routine
 *
 * @param irq an interrupt number
 *
 * @returns always 0
 *
 * @note handler return codes ignored for now
 *
 * XXX maybe we want to keep acking IRQs as in eirq_dispatch...
 */
__attribute__((unused))
static int leon_irq_dispatch(unsigned int irq)
{
	struct irl_vector_elem *p_elem;


#ifdef CONFIG_IRQ_STATS_COLLECT
	irqstat.irl++;
	irqstat.irl_irq[irq]++;
#endif /* CONFIG_IRQ_STATS_COLLECT */

	list_for_each_entry(p_elem, &irl_vector[irq], handler_node) {
		if (likely(p_elem->priority == ISR_PRIORITY_NOW))
			p_elem->handler(irq, p_elem->data);
		else if (leon_irq_queue(p_elem) < 0)
			p_elem->handler(irq, p_elem->data);
	}

	return 0;
}


/**
 * @brief the central interrupt handling routine for extended interrupts
 *
 * @param irq an extended interrupt number
 *
 * @returns always 0
 *
 * @note handler return codes ignored for now
 */
__attribute__((unused))
static int leon_eirq_dispatch(unsigned int irq)
{
	unsigned int eirq;
#ifdef CONFIG_LEON3
	unsigned int cpu;
#endif /* CONFIG_LEON3 */

	struct irl_vector_elem *p_elem;
	struct irl_vector_elem *p_tmp;


#ifdef CONFIG_IRQ_STATS_COLLECT
	irqstat.irl++;
	irqstat.irl_irq[irq]++;
#endif /* CONFIG_IRQ_STATS_COLLECT */

#ifdef CONFIG_LEON3
	cpu = leon3_cpuid();
#endif /* CONFIG_LEON3 */


	/* keep acknowleding pending EIRQs */
	/* XXX this is a potential death trap :) */
	while (1) {

#ifdef CONFIG_LEON3
		/* no pending EIRQs remain */
		if (!(leon_irqctrl_regs->irq_pending >> IRL_SIZE))
			break;

		eirq = leon_irqctrl_regs->extended_irq_id[cpu];
#endif /* CONFIG_LEON3 */

#ifdef CONFIG_LEON2
		eirq = leon_eirqctrl_regs->eirq_status;


		if (!(eirq & 0x20)) /* no pending EIRQs remain */
			break;
#endif /* CONFIG_LEON2 */

		eirq &= 0x1F;	/* get the actual EIRQ number */

		leon_clear_irq(LEON_WANT_EIRQ(eirq));


#ifdef CONFIG_IRQ_STATS_COLLECT
		irqstat.eirl++;
		irqstat.eirl_irq[eirq]++;
#endif /* CONFIG_IRQ_STATS_COLLECT */

		list_for_each_entry_safe(p_elem, p_tmp, &eirl_vector[eirq],
					 handler_node) {

			/* deferred ISRs are executed immediately if
			 * queueing fails
			 */
			if (likely(p_elem->priority == ISR_PRIORITY_NOW))
				p_elem->handler(eirq, p_elem->data);
			else if (leon_irq_queue(p_elem) < 0)
				p_elem->handler(eirq, p_elem->data);
		}
	}


	leon_clear_irq(irq);

	return 0;
}


/**
 * @brief	register a handler function to the primary interrupt line
 *
 * @param irq		an interrupt number
 * @param priority	a handler priority (immediate or deferred)
 * @param handler	a handler function
 * @param data	a pointer to arbitrary user data; passed to
 *			handler function
 *
 * @returns 0 on success
 */

int irl_register_handler(unsigned int irq,
			  enum isr_exec_priority priority,
			  irq_handler_t handler,
			  void *data)
{
	uint32_t psr_flags;

	struct irl_vector_elem *p_elem;


	if (list_empty(&irl_pool_head))
		return -ENOMEM;

	if (irq > IRL_SIZE)
		return -EINVAL;

	if (!handler)
		return -EINVAL;

	psr_flags = spin_lock_save_irq();

	p_elem = list_entry((&irl_pool_head)->next, struct irl_vector_elem,
			    handler_node);

	p_elem->handler  = handler;
	p_elem->priority = priority;
	p_elem->data     = data;
	p_elem->irq      = irq;

	list_move_tail(&p_elem->handler_node, &irl_vector[irq]);

	spin_lock_restore_irq(psr_flags);

#ifdef CONFIG_LEON3
	/* XXX for now, just register to the current CPU */
	leon_enable_irq(irq, leon3_cpuid());
#endif /* CONFIG_LEON3 */
#ifdef CONFIG_LEON2
	leon_enable_irq(irq, 0);
#endif /* CONFIG_LEON2 */


	/* here goes the call to whatever the low level handler is */
#ifdef CONFIG_ARCH_CUSTOM_BOOT_CODE
	pr_warn("NOT IMPLEMENTED: catch_interrupt() %s:%d\n", __FILE__, __LINE__);
	return -1;
#else
	/* provided by BCC/libgloss */
	return catch_interrupt(((int) leon_irq_dispatch), irq);
#endif /* CONFIG_ARCH_CUSTOM_BOOT_CODE */
}


/**
 * @brief	register a handler function to the extended interrupt line
 *
 * @param	IRQ		an extended interrupt number
 * @param	priority	a handler priority
 * @param	handler	a handler function
 * @param	data	a pointer to arbitrary user data
 *
 * @returns 0 on success
 *
 * XXX once this is confirmed working as planned, merge with function above,
 *     we only need one of those
 */

static int eirl_register_handler(unsigned int irq,
				 enum isr_exec_priority priority,
				 irq_handler_t handler,
				 void *data)
{
	uint32_t psr_flags;

	struct irl_vector_elem *p_elem;


	if (list_empty(&irl_pool_head))
		return -ENOMEM;

	if (irq > (IRL_SIZE + EIRL_SIZE))
		return -EINVAL;

	if (irq < IRL_SIZE)
		return -EINVAL;

	if (!handler)
		return -EINVAL;

	psr_flags = spin_lock_save_irq();

	p_elem = list_entry((&irl_pool_head)->next, struct irl_vector_elem,
		    handler_node);

	p_elem->handler  = handler;
	p_elem->priority = priority;
	p_elem->data     = data;
	p_elem->irq      = LEON_REAL_EIRQ(irq);

	list_move_tail(&p_elem->handler_node,
		       &eirl_vector[LEON_REAL_EIRQ(irq)]);

	spin_lock_restore_irq(psr_flags);
#ifdef CONFIG_LEON3
	/* XXX for now, just register to the current CPU */
	leon_enable_irq(irq, leon3_cpuid());
#endif /* CONFIG_LEON3 */
#ifdef CONFIG_LEON2
	leon_enable_irq(irq, 0);
#endif /* CONFIG_LEON2 */

	return 0;
}


/**
 * @brief	de-register a handler function to the primary interrupt line
 *
 * @param	IRQ		an interrupt number
 * @param	handler	a handler function
 * @param	data	a pointer to arbitrary user data
 *
 * @note	in case of duplicate handlers, ALL encountered will
 *		be removed
 * @returns 0 on success
 */

static int irl_deregister_handler(unsigned int irq,
				   irq_handler_t handler,
				   void *data)
{
	uint32_t psr_flags;

	struct irl_vector_elem *p_elem;
	struct irl_vector_elem *p_tmp;


	if (irq > 0xF)
		return -EINVAL;

	list_for_each_entry_safe(p_elem, p_tmp,
				 &irl_vector[irq], handler_node) {

		if (p_elem->handler == handler) {
			if (p_elem->data == data) {

				p_elem->handler  = NULL;
				p_elem->data = NULL;
				p_elem->priority = -1;

				psr_flags = spin_lock_save_irq();
				list_move_tail(&p_elem->handler_node,
					       &irl_pool_head);
				spin_lock_restore_irq(psr_flags);

				if (irl_vector[irq].next == &irl_vector[irq])
					leon_disable_irq(irq, leon3_cpuid());
			}
		}
	}

	return 0;
}



/**
 * @brief	de-register a handler function to the primary interrupt line
 *
 * @param	irq		an interrupt number
 * @param	handler	a handler function
 * @param	data	a pointer to arbitrary user data
 *
 * @note	in case of duplicate handlers, ALL encountered will
 *		be removed
 * @returns 0 on success
 *
 * XXX also think about merging with above
 */

static int eirl_deregister_handler(unsigned int irq,
				   irq_handler_t handler,
				   void *data)
{
	uint32_t psr_flags;

	struct irl_vector_elem *p_elem;
	struct irl_vector_elem *p_tmp;


	/* XXX magic: extnded interrupts are 16...31 */
	if (irq > 0x1F)
		return -EINVAL;

	if (irq < 0x10)
		return -EINVAL;

	list_for_each_entry_safe(p_elem, p_tmp,
				 &eirl_vector[irq], handler_node) {

		if (p_elem->handler == handler) {
			if (p_elem->data == data) {

				p_elem->handler  = NULL;
				p_elem->data = NULL;
				p_elem->priority = -1;

				psr_flags = spin_lock_save_irq();
				list_move_tail(&p_elem->handler_node,
					       &irl_pool_head);
				spin_lock_restore_irq(psr_flags);

				if (eirl_vector[irq].next == &eirl_vector[irq])
					leon_disable_irq(irq, leon3_cpuid());

			}
		}
	}

	return 0;
}


/**
 * @brief initialise some of the internals
 *
 * @returns 0 on success
 */

static int irq_dispatch_init(void)
{
	size_t i;

#ifdef CONFIG_IRQ_STATS_COLLECT
	struct sysset *sset;
	struct sysobj *sobj;
#endif /* CONFIG_IRQ_STATS_COLLECT */


	irl_vector = (struct list_head *)
			kzalloc(IRL_SIZE * sizeof(struct list_head));
	if (!irl_vector)
		return -1;


	eirl_vector = (struct list_head *)
			kzalloc(EIRL_SIZE * sizeof(struct list_head));
	if (!eirl_vector)
		return -1;

	irl_pool = (struct irl_vector_elem *)
			kzalloc(IRL_POOL_SIZE * sizeof(struct irl_vector_elem));
	if(!irl_pool)
		return -1;


	irl_queue_pool = (struct irl_vector_elem *)
		kzalloc(IRL_QUEUE_SIZE * sizeof(struct irl_vector_elem));
	if(!irl_queue_pool)
		return -1;



	INIT_LIST_HEAD(&irl_pool_head);
	INIT_LIST_HEAD(&irl_queue_head);
	INIT_LIST_HEAD(&irq_queue_pool_head);

	for (i = 0; i < IRL_SIZE; i++)
		INIT_LIST_HEAD(&irl_vector[i]);

	for (i = 0; i < EIRL_SIZE; i++)
		INIT_LIST_HEAD(&eirl_vector[i]);

	for (i = 0; i < IRL_POOL_SIZE; i++)
		list_add_tail(&irl_pool[i].handler_node, &irl_pool_head);

	for (i = 0; i < IRL_QUEUE_SIZE; i++)
		list_add_tail(&irl_queue_pool[i].handler_node,
			      &irq_queue_pool_head);

#ifdef CONFIG_IRQ_STATS_COLLECT
	sset = sysset_create_and_add("irl", NULL, sys_set);

	sobj = sysobj_create();

	if (!sobj)
		return -1;

	sobj->sattr = irl_attributes;
	sysobj_add(sobj, NULL, sset, "primary");

	sobj = sysobj_create();

	if (!sobj)
		return -1;

	sobj->sattr = eirl_attributes;
	sysobj_add(sobj, NULL, sset, "secondary");
#endif /* CONFIG_IRQ_STATS_COLLECT */

	return 0;
}


/**
 * @brief probe for extended IRQ controller
 */

static void leon_setup_eirq(void)
{
	unsigned int eirq;

#ifdef CONFIG_LEON3
	/* probe for extended IRQ controller, see GR712UM, p75 */
	eirq = (ioread32be(&leon_irqctrl_regs->mp_status) >> 16) & 0xf;
#endif /* CONFIG_LEON3 */
#ifdef CONFIG_LEON2
	eirq = 10;	/* yep... */
#endif /* CONFIG_LEON2 */

	if (!eirq || eirq > IRL_SIZE) {
		pr_err("IRQ: extended irq controller invalid: %d\n", eirq);
#if 1
		BUG();
#else
		return;
#endif
	}

	leon_eirq = eirq;
#ifdef CONFIG_ARCH_CUSTOM_BOOT_CODE
	pr_warn("NOT IMPLEMENTED: catch_interrupt() %s:%d\n", __FILE__, __LINE__);
#else
	/* provided by BCC/libgloss */
	BUG_ON(catch_interrupt((int) leon_eirq_dispatch, leon_eirq));
#endif /* CONFIG_ARCH_CUSTOM_BOOT_CODE */

#ifdef CONFIG_LEON3
	/* XXX for now, just register to the current CPU */
	leon_enable_irq(leon_eirq, leon3_cpuid());
#endif /* CONFIG_LEON3 */
#ifdef CONFIG_LEON2
	leon_enable_irq(leon_eirq, 0);
#endif /* CONFIG_LEON2 */

}


/**
 * @brief set interrupt hardware priority
 *
 * @param mask the interrupt selection mask to apply
 *
 * @param level selected interrupt level 1== HIGH, 0 == LOW
 */

__attribute__((unused))
static void leon_irq_set_level(uint32_t irq_mask, uint32_t level)
{
	uint32_t flags;


#ifdef CONFIG_LEON3
	flags = ioread32be(&leon_irqctrl_regs->irq_level);
#endif /* CONFIG_LEON3 */
#ifdef CONFIG_LEON2
	flags = ioread32be(&leon_irqctrl_regs->irq_mask) >> 17;
#endif /* CONFIG_LEON2 */

	if (!level)
		flags &= ~irq_mask;
	else
		flags |= irq_mask;

#ifdef CONFIG_LEON3
	iowrite32be(flags, &leon_irqctrl_regs->irq_level);
#endif /* CONFIG_LEON3 */
#ifdef CONFIG_LEON2
	iowrite32be(flags << 17, &leon_irqctrl_regs->irq_mask);
#endif /* CONFIG_LEON2 */

}

/**
 * @brief enable an IRQ
 *
 * @param data a struct irq_data
 */

static unsigned int enable_irq(struct irq_data *data)
{
	if (data->irq < IRL_SIZE)
		return irl_register_handler(data->irq, data->priority,
					     data->handler, data->data);

	return eirl_register_handler(data->irq, data->priority,
				     data->handler, data->data);
}

/**
 * @brief disable an IRQ
 *
 * @param data a struct irq_data
 */

static void disable_irq(struct irq_data *data)
{
	if (data->irq > 0xF) {
		eirl_deregister_handler(data->irq, data->handler, data->data);
	} else if (data->irq < 0x10) {
		irl_deregister_handler(data->irq, data->handler, data->data);
	}
}


/**
 * @brief mask an IRQ
 */

static void mask_irq(struct irq_data *data)
{
	leon_mask_irq(data->irq, leon3_cpuid());
}


/**
 * @brief mask an IRQ
 *
 * @param data a struct irq_data
 */

static void unmask_irq(struct irq_data *data)
{
	leon_unmask_irq(data->irq, leon3_cpuid());
}


/**
 * @brief mask an IRQ
 */

static void execute_deferred_irq(void)
{
	leon_irq_queue_execute();
}



/**
 * the configuration for the high-level irq manager
 */

static struct irq_dev leon_irq = {
	.irq_enable	= enable_irq,
	.irq_disable	= disable_irq,
	.irq_mask	= mask_irq,
	.irq_unmask	= unmask_irq,
	.irq_deferred	= execute_deferred_irq,
};


/**
 * @brief initialise the IRQ subsystem on the LEON
 */

void leon_irq_init(void)
{
#ifdef CONFIG_LEON3
	/* XXX should determine that from AMBA PNP scan */
	leon_irqctrl_regs = (struct leon3_irqctrl_registermap *)
						LEON3_BASE_ADDRESS_IRQMP;

	/* mask all interrupts on this (boot) CPU */
	iowrite32be(0, &leon_irqctrl_regs->irq_mpmask[leon3_cpuid()]);
#endif /* CONFIG_LEON3 */
#ifdef CONFIG_LEON2
	leon_irqctrl_regs  = (struct leon2_irqctrl_registermap *)
						LEON2_BASE_ADDRESS_IRQ;
	leon_eirqctrl_regs = (struct leon2_eirqctrl_registermap *)
						LEON2_BASE_ADDRESS_EIRQ;


	iowrite32be(0, &leon_irqctrl_regs->irq_mask);
#endif /* CONFIG_LEON2 */

	irq_init(&leon_irq);

	BUG_ON(irq_dispatch_init());

	/* set up extended interrupt controller if found */
	leon_setup_eirq();
}
