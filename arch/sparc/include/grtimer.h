/**
 * @file   arch/sparc/grtimer.h
 * @ingroup time
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
 * @brief Provides access to the LEON3 General Purpose Timer Unit with
 *        Time Latch Capability
 *
 */

#ifndef _SPARC_GRTIMER_H_
#define _SPARC_GRTIMER_H_

#include <asm/leon_reg.h>


#define LEON3_GRTIMER_CFG_LATCH		0x800

#define LEON3_TIMER_EN	0x00000001       /* enable counting */
#define LEON3_TIMER_RS	0x00000002       /* restart from timer reload value */
#define LEON3_TIMER_LD	0x00000004       /* load counter    */
#define LEON3_TIMER_IE	0x00000008       /* irq enable      */
#define LEON3_TIMER_IP	0x00000010       /* irq pending (clear by writing 0 */
#define LEON3_TIMER_CH	0x00000020       /* chain with preceeding timer */

#define LEON3_CFG_TIMERS_MASK	0x00000007
#define LEON3_CFG_IRQNUM_MASK	0x000000f8
#define LEON3_CFG_IRQNUM_SHIFT	       0x3



/**
 * @brief set scaler reload value of the timer block
 * @param rtu a struct grtimer_unit
 * @param value the scaler reload value to set
 */
void grtimer_set_scaler_reload(struct grtimer_unit *rtu, uint32_t value);

/**
 * @brief get scaler reload value of the timer block
 * @param rtu a struct grtimer_unit
 *
 * @return the scaler reload value
 */
uint32_t grtimer_get_scaler_reload(struct grtimer_unit *rtu);

/**
 * @brief set the interrupt enabled flag of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */
void grtimer_set_interrupt_enabled(struct grtimer_unit *rtu, uint32_t timer);

/**
 * @brief clear the interrupt enabled flag of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */
void grtimer_clear_interrupt_enabled(struct grtimer_unit *rtu, uint32_t timer);

/**
 * @brief set the load flag of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */
void grtimer_set_load(struct grtimer_unit *rtu, uint32_t timer);

/**
 * @brief clear the load flag of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */
void grtimer_clear_load(struct grtimer_unit *rtu, uint32_t timer);

/**
 * @brief set enable flag in timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */
void grtimer_set_enabled(struct grtimer_unit *rtu, uint32_t timer);

/**
 * @brief clear enable flag in timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */
void grtimer_clear_enabled(struct grtimer_unit *rtu, uint32_t timer);

/**
 * @brief set restart flag in timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */
void grtimer_set_restart(struct grtimer_unit *rtu, uint32_t timer);

/**
 * @brief clear restart flag in timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */
void grtimer_clear_restart(struct grtimer_unit *rtu, uint32_t timer);

/**
 * @brief set timer to chain to the preceding timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */
void grtimer_set_chained(struct grtimer_unit *rtu, uint32_t timer);

/**
 * @brief clear timer chain to preceding timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */
void grtimer_clear_chained(struct grtimer_unit *rtu, uint32_t timer);

/**
 * @brief get interrupt pending status of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 *
 * @return the interrupt pending status flag
 */
uint32_t grtimer_get_interrupt_pending_status(struct grtimer_unit *rtu,
					      uint32_t timer);

/**
 * @brief clear interrupt pending status of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 */
void grtimer_clear_interrupt_pending_status(struct grtimer_unit *rtu,
					    uint32_t timer);

/**
 * @brief get number of implemented general purpose timers
 * @param rtu a struct grtimer_unit
 *
 * @return the number of implemented timers
 */
uint32_t grtimer_get_num_implemented(struct grtimer_unit *rtu);

/**
 * @brief get interrupt ID of first implemented timer
 * @param rtu a struct grtimer_unit
 *
 * @return the interrupt ID of the first implemented timer
 */
uint32_t grtimer_get_first_timer_irq_id(struct grtimer_unit *rtu);

/**
 * @brief set the counter value of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 * @param value the timer counter value to set
 */
void grtimer_set_value(struct grtimer_unit *rtu,
		       uint32_t timer,
		       uint32_t value);

/**
 * @brief get the counter value of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 *
 * @return the timer counter value
 */
uint32_t grtimer_get_value(struct grtimer_unit *rtu, uint32_t timer);

/**
 * @brief set the reload value of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 * @param reload the timer counter reload to set
 */
void grtimer_set_reload(struct grtimer_unit *rtu,
			uint32_t timer,
			uint32_t reload);

/**
 * @brief get the reload value of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 *
 * @return the timer counter reload value
 */
uint32_t grtimer_get_reload(struct grtimer_unit *rtu, uint32_t timer);

/**
 * @brief set an irq to trigger a latch
 * @param rtu a struct grtimer_unit
 * @param irq the irq number to latch on
 */
void grtimer_set_latch_irq(struct grtimer_unit *rtu, uint32_t irq);

/**
 * @brief clear an irq triggering a latch
 * @param rtu a struct grtimer_unit
 * @param irq the irq number to disable latching for
 */
void grtimer_clear_latch_irq(struct grtimer_unit *rtu, uint32_t irq);

/**
 * @brief enable the timer's latch capability
 * @param rtu a struct grtimer_unit
 */
void grtimer_enable_latch(struct grtimer_unit *rtu);

/**
 * @brief get the latch value of a timer
 * @param rtu a struct grtimer_unit
 * @param timer the selected timer
 *
 * @return the latched timer counter value
 */
uint32_t grtimer_get_latch_value(struct grtimer_unit *rtu, uint32_t timer);

#endif /* _SPARC_GRTIMER_H */
