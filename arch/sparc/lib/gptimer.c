/**
 * @file   arch/sparc/lib/gptimer.c
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
 * @brief implements access to the LEON3 General Purpose Timer Unit
 * @see GR712RC user manual chapter 11
 *
 */


#include <asm/io.h>
#include <gptimer.h>


/**
 * @brief set scaler reload value of the timer block
 * @param ptu a struct gptimer_unit
 *
 */

void gptimer_set_scaler_reload(struct gptimer_unit *ptu, uint32_t value)
{
	iowrite32be(value, &ptu->scaler_reload);
}


/**
 * @brief get scaler reload value of the timer block
 * @param ptu a struct gptimer_unit
 *
 */

uint32_t gptimer_get_scaler_reload(struct gptimer_unit *ptu)
{
	return ioread32be(&ptu->scaler_reload);
}


/**
 * @brief sets the interrupt enabled flag of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */

void gptimer_set_interrupt_enabled(struct gptimer_unit *ptu, uint32_t timer)
{
	uint32_t flags;

	flags  = ioread32be(&ptu->timer[timer].ctrl);
	flags |= LEON3_TIMER_IE;

	iowrite32be(flags, &ptu->timer[timer].ctrl);
}


/**
 * @brief sets the interrupt enabled flag of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */

void gptimer_clear_interrupt_enabled(struct gptimer_unit *ptu, uint32_t timer)
{
	uint32_t flags;

	flags  = ioread32be(&ptu->timer[timer].ctrl);
	flags &= ~LEON3_TIMER_IE;

	iowrite32be(flags, &ptu->timer[timer].ctrl);
}


/**
 * @brief sets the load flag of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */

void gptimer_set_load(struct gptimer_unit *ptu, uint32_t timer)
{
	uint32_t flags;

	flags  = ioread32be(&ptu->timer[timer].ctrl);
	flags |= LEON3_TIMER_LD;

	iowrite32be(flags, &ptu->timer[timer].ctrl);
}


/**
 * @brief clears the load flag of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */

void gptimer_clear_load(struct gptimer_unit *ptu, uint32_t timer)
{
	uint32_t flags;

	flags  = ioread32be(&ptu->timer[timer].ctrl);
	flags &= ~LEON3_TIMER_LD;

	iowrite32be(flags, &ptu->timer[timer].ctrl);
}


/**
 * @brief set enable flag in timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */
void gptimer_set_enabled(struct gptimer_unit *ptu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&ptu->timer[timer].ctrl);
	ctrl |= LEON3_TIMER_EN;

	iowrite32be(ctrl, &ptu->timer[timer].ctrl);
}


/**
 * @brief clear enable flag in timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */

void gptimer_clear_enabled(struct gptimer_unit *ptu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&ptu->timer[timer].ctrl);
	ctrl &= ~LEON3_TIMER_EN;

	iowrite32be(ctrl, &ptu->timer[timer].ctrl);
}


/**
 * @brief set restart flag in timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */
void gptimer_set_restart(struct gptimer_unit *ptu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&ptu->timer[timer].ctrl);
	ctrl |= LEON3_TIMER_RS;

	iowrite32be(ctrl, &ptu->timer[timer].ctrl);
}


/**
 * @brief clear restart flag in timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */

void gptimer_clear_restart(struct gptimer_unit *ptu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&ptu->timer[timer].ctrl);
	ctrl &= ~LEON3_TIMER_RS;

	iowrite32be(ctrl, &ptu->timer[timer].ctrl);
}


/**
 * @brief set timer to chain to the preceeding timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */

void gptimer_set_chained(struct gptimer_unit *ptu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&ptu->timer[timer].ctrl);
	ctrl |= LEON3_TIMER_CH;

	iowrite32be(ctrl, &ptu->timer[timer].ctrl);
}


/**
 * @brief clear timer to chain to the preceeding timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */

void gptimer_clear_chained(struct gptimer_unit *ptu, uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&ptu->timer[timer].ctrl);
	ctrl &= ~LEON3_TIMER_CH;

	iowrite32be(ctrl, &ptu->timer[timer].ctrl);
}


/**
 * @brief get status of interrupt pending status
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */

uint32_t gptimer_get_interrupt_pending_status(struct gptimer_unit *ptu,
					      uint32_t timer)
{
	return ioread32be(&ptu->timer[timer].ctrl) & LEON3_TIMER_IP;
}


/**
 * @brief clear status of interrupt pending status
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */

void gptimer_clear_interrupt_pending_status(struct gptimer_unit *ptu,
					    uint32_t timer)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&ptu->timer[timer].ctrl);
	ctrl &= ~LEON3_TIMER_IP;

	iowrite32be(ctrl, &ptu->timer[timer].ctrl);
}


/**
 * @brief get number of implemented general purpose timers
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */

uint32_t gptimer_get_num_implemented(struct gptimer_unit *ptu)
{
	return ioread32be(&ptu->config) & LEON3_CFG_TIMERS_MASK;
}


/**
 * @brief get interrupt ID of first implemented timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */

uint32_t gptimer_get_first_timer_irq_id(struct gptimer_unit *ptu)
{
	return (ioread32be(&ptu->config) & LEON3_CFG_IRQNUM_MASK) >>
		LEON3_CFG_IRQNUM_SHIFT;
}


/**
 * @brief set the value of a gptimer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 * @param value the timer counter value to set
 */

void gptimer_set_value(struct gptimer_unit *ptu, uint32_t timer, uint32_t value)
{
	iowrite32be(value, &ptu->timer[timer].value);
}

/**
 * @brief get the value of a gptimer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 * @param value the timer counter value to set
 */

uint32_t gptimer_get_value(struct gptimer_unit *ptu, uint32_t timer)
{
	return ioread32be(&ptu->timer[timer].value);
}


/**
 * @brief set the reload of a gptimer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 * @param reload the timer counter reload to set
 */

void gptimer_set_reload(struct gptimer_unit *ptu,
			uint32_t timer,
			uint32_t reload)
{
	iowrite32be(reload, &ptu->timer[timer].reload);
}

/**
 * @brief get the reload of a gptimer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 * @param reload the timer counter reload to set
 */

uint32_t gptimer_get_reload(struct gptimer_unit *ptu, uint32_t timer)
{
	return ioread32be(&ptu->timer[timer].reload);
}


/**
 * @brief starts a gptimer; emits an irq but does not enable reload on underflow
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 * @param value the timer counter value to set
 */

void gptimer_start(struct gptimer_unit *ptu, uint32_t timer, uint32_t value)
{
	gptimer_clear_enabled(ptu, timer);
	gptimer_clear_restart(ptu, timer);

	gptimer_set_value(ptu, timer, value);
	gptimer_set_reload(ptu, timer, value);

	gptimer_set_interrupt_enabled(ptu, timer);
	gptimer_set_load(ptu, timer);
	gptimer_set_enabled(ptu, timer);
}


/**
 * @brief start a gptimer, emits an irq and enables reload on underflow
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 * @param value the timer counter value to set
 */

void gptimer_start_cyclical(struct gptimer_unit *ptu,
			    uint32_t timer, uint32_t value)
{
	gptimer_clear_enabled(ptu, timer);
	gptimer_clear_restart(ptu, timer);

	gptimer_set_value(ptu, timer, value);
	gptimer_set_reload(ptu, timer, value);

	gptimer_set_interrupt_enabled(ptu, timer);
	gptimer_set_load(ptu, timer);
	gptimer_set_restart(ptu, timer);
	gptimer_set_enabled(ptu, timer);
}
