/**
 * @file   arch/sparc/grtimer.h
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



void grtimer_set_scaler_reload(struct grtimer_unit *rtu, uint32_t value);
uint32_t grtimer_get_scaler_reload(struct grtimer_unit *rtu);

void grtimer_set_interrupt_enabled(struct grtimer_unit *rtu, uint32_t timer);
void grtimer_clear_interrupt_enabled(struct grtimer_unit *rtu, uint32_t timer);

void grtimer_set_load(struct grtimer_unit *rtu, uint32_t timer);
void grtimer_clear_load(struct grtimer_unit *rtu, uint32_t timer);

void grtimer_set_enabled(struct grtimer_unit *rtu, uint32_t timer);
void grtimer_clear_enabled(struct grtimer_unit *rtu, uint32_t timer);

void grtimer_set_restart(struct grtimer_unit *rtu, uint32_t timer);
void grtimer_clear_restart(struct grtimer_unit *rtu, uint32_t timer);

void grtimer_set_chained(struct grtimer_unit *rtu, uint32_t timer);
void grtimer_clear_chained(struct grtimer_unit *rtu, uint32_t timer);

uint32_t grtimer_get_interrupt_pending_status(struct grtimer_unit *rtu,
					      uint32_t timer);
void grtimer_clear_interrupt_pending_status(struct grtimer_unit *rtu,
					    uint32_t timer);

uint32_t grtimer_get_num_implemented(struct grtimer_unit *rtu);

uint32_t grtimer_get_first_timer_irq_id(struct grtimer_unit *rtu);

void grtimer_set_value(struct grtimer_unit *rtu,
		       uint32_t timer,
		       uint32_t value);
uint32_t grtimer_get_value(struct grtimer_unit *rtu, uint32_t timer);


void grtimer_set_reload(struct grtimer_unit *rtu,
			uint32_t timer,
			uint32_t reload);
uint32_t grtimer_get_reload(struct grtimer_unit *rtu, uint32_t timer);

void grtimer_set_latch_irq(struct grtimer_unit *rtu, uint32_t irq);
void grtimer_clear_latch_irq(struct grtimer_unit *rtu, uint32_t irq);
void grtimer_enable_latch(struct grtimer_unit *rtu);

uint32_t grtimer_get_latch_value(struct grtimer_unit *rtu, uint32_t timer);

#endif /* _SPARC_GRTIMER_H */
