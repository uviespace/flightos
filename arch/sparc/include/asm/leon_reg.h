/**
 * @file arch/sparc/include/asm/leon_reg.h
 *
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 * @date    2015
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
 *
 */

#ifndef _SPARC_ASM_LEON_REG_H_
#define _SPARC_ASM_LEON_REG_H_

#include <kernel/types.h>



#define LEON3_BASE_ADDRESS_APB		0x80000000

#define LEON3_BASE_ADDRESS_FTMCTRL	0x80000000
#define LEON3_BASE_ADDRESS_APBUART	0x80000100
#define LEON3_BASE_ADDRESS_IRQMP	0x80000200
#define LEON3_BASE_ADDRESS_GPTIMER	0x80000300
#define LEON3_BASE_ADDRESS_GRGPIO_1	0x80000900
#define LEON3_BASE_ADDRESS_GRGPIO_2	0x80000A00
#define LEON3_BASE_ADDRESS_AHBSTAT	0x80000F00
#define LEON3_BASE_ADDRESS_GRTIMER	0x80100600

#define LEON3_IRL_SIZE	16 /* number of interrupts on primary */
#define LEON3_EIRL_SIZE	16 /* number of interrupts on extended */





#define LEON2_BASE_ADDRESS_IRQ		0x80000090
#define LEON2_BASE_ADDRESS_EIRQ		0x800000B0

#define LEON2_IRL_SIZE	16 /* number of interrupts on primary */
#define LEON2_EIRL_SIZE	32 /* number of interrupts on extended */



struct leon2_irqctrl_registermap {
        uint32_t irq_mask;
        uint32_t irq_pending;
        uint32_t irq_force;
        uint32_t irq_clear;
};

struct leon2_eirqctrl_registermap {
        uint32_t eirq_mask;
        uint32_t eirq_pending;
        uint32_t eirq_status;
        uint32_t eirq_clear;
};





/* FTMCTRL Memory Controller Registers [p. 44] */

struct leon3_ftmctrl_registermap {
	 uint32_t mcfg1;
	 uint32_t mcfg2;
	 uint32_t mcfg3;
};

struct leon3_apbuart_registermap {
	 uint32_t data;
	 uint32_t status;
	 uint32_t ctrl;
	 uint32_t scaler;
};

struct leon3_grgpio_registermap {
	uint32_t ioport_data;
	uint32_t ioport_output_value;
	uint32_t ioport_direction;
	uint32_t irq_mask;
	uint32_t irq_polarity;
	uint32_t irq_edge;
};

struct leon3_irqctrl_registermap {
	uint32_t irq_level;			/* 0x00	*/
	uint32_t irq_pending;			/* 0x04 */
	uint32_t irq_force;			/* 0x08 */
	uint32_t irq_clear;			/* 0x0C */
	uint32_t mp_status;			/* 0x10 */
	uint32_t mp_broadcast;			/* 0x14 */
	uint32_t unused01;			/* 0x18 */
	uint32_t unused02;			/* 0x1C */
	uint32_t unused03;			/* 0x20 */
	uint32_t unused04;			/* 0x24 */
	uint32_t unused05;			/* 0x28 */
	uint32_t unused06;			/* 0x2C */
	uint32_t unused07;			/* 0x30 */
	uint32_t unused08;			/* 0x34 */
	uint32_t unused09;			/* 0x38 */
	uint32_t unused10;			/* 0x3C */
	uint32_t irq_mpmask[2];			/* 0x40 CPU 0 */
						/* 0x44 CPU 1 */
	uint32_t unused11;			/* 0x48 */
	uint32_t unused12;			/* 0x4C */
	uint32_t unused13;			/* 0x50 */
	uint32_t unused14;			/* 0x54 */
	uint32_t unused15;			/* 0x58 */
	uint32_t unused16;			/* 0x5C */
	uint32_t unused17;			/* 0x60 */
	uint32_t unused18;			/* 0x64 */
	uint32_t unused19;			/* 0x68 */
	uint32_t unused20;			/* 0x6C */
	uint32_t unused21;			/* 0x70 */
	uint32_t unused22;			/* 0x74 */
	uint32_t unused23;			/* 0x78 */
	uint32_t unused24;			/* 0x7C */
	uint32_t irq_mpforce[2];		/* 0x80 CPU 0*/
						/* 0x84 CPU 1*/
	uint32_t unused25;			/* 0x88 */
	uint32_t unused26;			/* 0x8C */
	uint32_t unused27;			/* 0x90 */
	uint32_t unused28;			/* 0x94 */
	uint32_t unused29;			/* 0x98 */
	uint32_t unused30;			/* 0x9C */
	uint32_t unused31;			/* 0xA0 */
	uint32_t unused32;			/* 0xA4 */
	uint32_t unused33;			/* 0xA8 */
	uint32_t unused34;			/* 0xAC */
	uint32_t unused35;			/* 0xB0 */
	uint32_t unused36;			/* 0xB4 */
	uint32_t unused37;			/* 0xB8 */
	uint32_t unused38;			/* 0xBC */
	uint32_t extended_irq_id[2];		/* 0xC0 CPU 0*/
						/* 0xC4 CPU 1*/
};



struct leon3_ahbstat_registermap {
	uint32_t status;
	uint32_t failing_address;
};



struct gptimer_timer {
	uint32_t value;
	uint32_t reload;
	uint32_t ctrl;
	uint32_t unused;
};

struct gptimer_unit {
	uint32_t scaler;
	uint32_t scaler_reload;
	uint32_t config;
	uint32_t unused;
	struct gptimer_timer timer[4];
};

struct grtimer_timer {
	uint32_t value;
	uint32_t reload;
	uint32_t ctrl;
	uint32_t latch_value;
};

struct grtimer_unit {
	uint32_t scaler;
	uint32_t scaler_reload;
	uint32_t config;

	uint32_t irq_select;

	struct grtimer_timer timer[2];
};

#endif /* _SPARC_ASM_LEON_REG_H_ */
