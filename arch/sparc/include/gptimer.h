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


void gptimer_set_scaler_reload(struct gptimer_unit *ptu, uint32_t value);
uint32_t gptimer_get_scaler_reload(struct gptimer_unit *ptu);

void gptimer_set_interrupt_enabled(struct gptimer_unit *ptu, uint32_t timer);
void gptimer_clear_interrupt_enabled(struct gptimer_unit *ptu, uint32_t timer);

void gptimer_set_load(struct gptimer_unit *ptu, uint32_t timer);
void gptimer_clear_load(struct gptimer_unit *ptu, uint32_t timer);

void gptimer_set_enabled(struct gptimer_unit *ptu, uint32_t timer);
void gptimer_clear_enabled(struct gptimer_unit *ptu, uint32_t timer);

void gptimer_set_restart(struct gptimer_unit *ptu, uint32_t timer);
void gptimer_clear_restart(struct gptimer_unit *ptu, uint32_t timer);

void gptimer_set_chained(struct gptimer_unit *ptu, uint32_t timer);
void gptimer_clear_chained(struct gptimer_unit *ptu, uint32_t timer);

uint32_t gptimer_get_interrupt_pending_status(struct gptimer_unit *ptu,
					      uint32_t timer);
void gptimer_clear_interrupt_pending_status(struct gptimer_unit *ptu,
					    uint32_t timer);

uint32_t gptimer_get_num_implemented(struct gptimer_unit *ptu);
uint32_t gptimer_get_first_timer_irq_id(struct gptimer_unit *ptu);

void gptimer_set_value(struct gptimer_unit *ptu,
		       uint32_t timer,
		       uint32_t value);
uint32_t gptimer_get_value(struct gptimer_unit *ptu, uint32_t timer);

void gptimer_set_reload(struct gptimer_unit *ptu,
			uint32_t timer,
			uint32_t reload);
uint32_t gptimer_get_reload(struct gptimer_unit *ptu, uint32_t timer);

void gptimer_start(struct gptimer_unit *ptu, uint32_t timer, uint32_t value);

void gptimer_start_cyclical(struct gptimer_unit *ptu,
			    uint32_t timer,
			    uint32_t value);

#endif /* _SPARC_GPTIMER_H */
