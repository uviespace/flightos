/**
 * @file   arch/sparc/gptimer.h
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
 * @brief Provides access to the LEON3 General Purpose Timer Unit
 *
 */

#ifndef _SPARC_GPTIMER_H
#define _SPARC_GPTIMER_H

#include <asm/leon_reg.h>


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
 * @param ptu a struct gptimer_unit
 * @param value the scaler reload value to set
 */
void gptimer_set_scaler_reload(struct gptimer_unit *ptu, uint32_t value);

/**
 * @brief get scaler reload value of the timer block
 * @param ptu a struct gptimer_unit
 *
 * @return the scaler reload value
 */
uint32_t gptimer_get_scaler_reload(struct gptimer_unit *ptu);

/**
 * @brief set the interrupt enabled flag of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */
void gptimer_set_interrupt_enabled(struct gptimer_unit *ptu, uint32_t timer);

/**
 * @brief clear the interrupt enabled flag of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */
void gptimer_clear_interrupt_enabled(struct gptimer_unit *ptu, uint32_t timer);

/**
 * @brief set the load flag of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */
void gptimer_set_load(struct gptimer_unit *ptu, uint32_t timer);

/**
 * @brief clear the load flag of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */
void gptimer_clear_load(struct gptimer_unit *ptu, uint32_t timer);

/**
 * @brief set enable flag in timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */
void gptimer_set_enabled(struct gptimer_unit *ptu, uint32_t timer);

/**
 * @brief clear enable flag in timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */
void gptimer_clear_enabled(struct gptimer_unit *ptu, uint32_t timer);

/**
 * @brief set restart flag in timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */
void gptimer_set_restart(struct gptimer_unit *ptu, uint32_t timer);

/**
 * @brief clear restart flag in timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */
void gptimer_clear_restart(struct gptimer_unit *ptu, uint32_t timer);

/**
 * @brief set timer to chain to the preceding timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */
void gptimer_set_chained(struct gptimer_unit *ptu, uint32_t timer);

/**
 * @brief clear timer chain to preceding timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */
void gptimer_clear_chained(struct gptimer_unit *ptu, uint32_t timer);

/**
 * @brief get interrupt pending status of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 *
 * @return the interrupt pending status flag
 */
uint32_t gptimer_get_interrupt_pending_status(struct gptimer_unit *ptu,
					      uint32_t timer);

/**
 * @brief clear interrupt pending status of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 */
void gptimer_clear_interrupt_pending_status(struct gptimer_unit *ptu,
					    uint32_t timer);

/**
 * @brief get number of implemented general purpose timers
 * @param ptu a struct gptimer_unit
 *
 * @return the number of implemented timers
 */
uint32_t gptimer_get_num_implemented(struct gptimer_unit *ptu);

/**
 * @brief get interrupt ID of first implemented timer
 * @param ptu a struct gptimer_unit
 *
 * @return the interrupt ID of the first implemented timer
 */
uint32_t gptimer_get_first_timer_irq_id(struct gptimer_unit *ptu);

/**
 * @brief set the counter value of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 * @param value the timer counter value to set
 */
void gptimer_set_value(struct gptimer_unit *ptu,
		       uint32_t timer,
		       uint32_t value);

/**
 * @brief get the counter value of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 *
 * @return the timer counter value
 */
uint32_t gptimer_get_value(struct gptimer_unit *ptu, uint32_t timer);

/**
 * @brief set the reload value of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 * @param reload the timer counter reload to set
 */
void gptimer_set_reload(struct gptimer_unit *ptu,
			uint32_t timer,
			uint32_t reload);

/**
 * @brief get the reload value of a timer
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 *
 * @return the timer counter reload value
 */
uint32_t gptimer_get_reload(struct gptimer_unit *ptu, uint32_t timer);

/**
 * @brief start a timer in one-shot mode without reload on underflow
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 * @param value the timer counter value to set
 */
void gptimer_start(struct gptimer_unit *ptu, uint32_t timer, uint32_t value);

/**
 * @brief start a timer in cyclical mode with reload on underflow
 * @param ptu a struct gptimer_unit
 * @param timer the selected timer
 * @param value the timer counter value to set
 */
void gptimer_start_cyclical(struct gptimer_unit *ptu,
			    uint32_t timer,
			    uint32_t value);

#endif /* _SPARC_GPTIMER_H */
