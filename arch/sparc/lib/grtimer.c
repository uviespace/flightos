/**
 * @file   arch/sparc/lib/grtimer.c
 * @ingroup time
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
 *
 * @brief Implements access to the LEON3 General Purpose Timer Unit with
 *	  Time Latch Capability
 *
 * @see GR712RC user manual chapter 12
 */


#include <asm/io.h>
#include <grtimer.h>


/**
 * @brief set scaler reload value of the timer block
 * @param rtu a struct grtimer_unit
 *
 */

void grtimer_set_scaler_reload(struct grtimer_unit *rtu, uint32_t value)
{
	iowrite32be(value, &rtu->scaler_reload);
}


/**
 * @brief get scaler reload value of the timer block
 * @param rtu a struct grtimer_unit
 *
 */

uint32_t grtimer_get_scaler_reload(struct grtimer_unit *rtu)
{
	return ioread32be(&rtu->scaler_reload);
}


/**
 * @brief sets the interrupt enabled flag of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

void grtimer_set_interrupt_enabled(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t flags;

	flags  = ioread32be(&rtu->timer[timer].ctrl);
	flags |= LEON3_TIMER_IE;

	iowrite32be(flags, &rtu->timer[timer].ctrl);
}


/**
 * @brief sets the interrupt enabled flag of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

void grtimer_clear_interrupt_enabled(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t flags;

	flags  = ioread32be(&rtu->timer[timer].ctrl);
	flags &= ~LEON3_TIMER_IE;

	iowrite32be(flags, &rtu->timer[timer].ctrl);
}


/**
 * @brief sets the load flag of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

void grtimer_set_load(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t flags;

	flags  = ioread32be(&rtu->timer[timer].ctrl);
	flags |= LEON3_TIMER_LD;

	iowrite32be(flags, &rtu->timer[timer].ctrl);
}


/**
 * @brief clears the load flag of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

void grtimer_clear_load(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t flags;

	flags  = ioread32be(&rtu->timer[timer].ctrl);
	flags &= ~LEON3_TIMER_LD;

	iowrite32be(flags, &rtu->timer[timer].ctrl);
}


/**
 * @brief set enable flag in timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

void grtimer_set_enabled(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&rtu->timer[timer].ctrl);
	ctrl |= LEON3_TIMER_EN;

	iowrite32be(ctrl, &rtu->timer[timer].ctrl);
}


/**
 * @brief clear enable flag in timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

void grtimer_clear_enabled(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&rtu->timer[timer].ctrl);
	ctrl &= ~LEON3_TIMER_EN;

	iowrite32be(ctrl, &rtu->timer[timer].ctrl);
}


/**
 * @brief set restart flag in timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

void grtimer_set_restart(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&rtu->timer[timer].ctrl);
	ctrl |= LEON3_TIMER_RS;

	iowrite32be(ctrl, &rtu->timer[timer].ctrl);
}


/**
 * @brief clear restart flag in timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

void grtimer_clear_restart(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&rtu->timer[timer].ctrl);
	ctrl &= ~LEON3_TIMER_RS;

	iowrite32be(ctrl, &rtu->timer[timer].ctrl);
}


/**
 * @brief set timer to chain to the preceeding timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

void grtimer_set_chained(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&rtu->timer[timer].ctrl);
	ctrl |= LEON3_TIMER_CH;

	iowrite32be(ctrl, &rtu->timer[timer].ctrl);
}


/**
 * @brief clear timer to chain to the preceeding timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

void grtimer_clear_chained(struct grtimer_unit *rtu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&rtu->timer[timer].ctrl);
	ctrl &= ~LEON3_TIMER_CH;

	iowrite32be(ctrl, &rtu->timer[timer].ctrl);
}


/**
 * @brief get status of interrupt pending status
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

uint32_t grtimer_get_interrupt_pending_status(struct grtimer_unit *rtu,
					      uint32_t timer)
{
	return ioread32be(&rtu->timer[timer].ctrl) & LEON3_TIMER_IP;
}


/**
 * @brief clear status of interrupt pending status
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

void grtimer_clear_interrupt_pending_status(struct grtimer_unit *rtu,
					    uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&rtu->timer[timer].ctrl);
	ctrl &= ~LEON3_TIMER_IP;

	iowrite32be(ctrl, &rtu->timer[timer].ctrl);
}


/**
 * @brief get number of implemented general purpose timers
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

uint32_t grtimer_get_num_implemented(struct grtimer_unit *rtu)
{
	return ioread32be(&rtu->config) & LEON3_CFG_TIMERS_MASK;
}


/**
 * @brief get interrupt ID of first implemented timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

uint32_t grtimer_get_first_timer_irq_id(struct grtimer_unit *rtu)
{
	return (ioread32be(&rtu->config) & LEON3_CFG_IRQNUM_MASK) >>
		LEON3_CFG_IRQNUM_SHIFT;
}


/**
 * @brief set the value of a grtimer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 * @param value the timer counter value to set
 */

void grtimer_set_value(struct grtimer_unit *rtu, uint32_t timer, uint32_t value)
{
	iowrite32be(value, &rtu->timer[timer].value);
}

/**
 * @brief get the value of a grtimer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 * @param value the timer counter value to set
 */

uint32_t grtimer_get_value(struct grtimer_unit *rtu, uint32_t timer)
{
	return ioread32be(&rtu->timer[timer].value);
}


/**
 * @brief set the reload of a grtimer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 * @param reload the timer counter reload to set
 */

void grtimer_set_reload(struct grtimer_unit *rtu,
			uint32_t timer,
			uint32_t reload)
{
	iowrite32be(reload, &rtu->timer[timer].reload);
}

/**
 * @brief get the reload of a grtimer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

uint32_t grtimer_get_reload(struct grtimer_unit *rtu, uint32_t timer)
{
	return ioread32be(&rtu->timer[timer].reload);
}

/**
 * @brief set an irq to trigger a latch
 * @param rtu a struct grtimer_unit
 * @param irq the irq number to latch on
 */

void grtimer_set_latch_irq(struct grtimer_unit *rtu, uint32_t irq)
{
	uint32_t irq_select;

	irq_select  = ioread32be(&rtu->irq_select);
	irq_select |= (1 << irq);

	iowrite32be(irq_select, &rtu->irq_select);
}


/**
 * @brief clear an irq triggering a latch
 * @param rtu a struct grtimer_unit
 * @param irq the irq number to disable latching for
 */

void grtimer_clear_latch_irq(struct grtimer_unit *rtu, uint32_t irq)
{
	uint32_t irq_select;

	irq_select  = ioread32be(&rtu->irq_select);
	irq_select &= ~(1 << irq);

	iowrite32be(irq_select, &rtu->irq_select);
}


/**
 * @brief set the timer's latch bit
 * @param rtu a struct grtimer_unit
 */

void grtimer_enable_latch(struct grtimer_unit *rtu)
{
	uint32_t config;

	config  = ioread32be(&rtu->config);
	config |= LEON3_GRTIMER_CFG_LATCH;

	iowrite32be(config, &rtu->config);
}

/**
 * @brief get the latch value of a grtimer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */

uint32_t grtimer_get_latch_value(struct grtimer_unit *rtu, uint32_t timer)
{
	return ioread32be(&rtu->timer[timer].latch_value);
}
