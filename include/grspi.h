/**
 * @file   grspi.h
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   2026
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

#ifndef GRSPI_H
#define GRSPI_H

#include <kernel/types.h>

struct grspi;


/* see GR712RC-UM Mar 2025, Version 2.17, pp.175 */
struct grspi_regs {
        uint32_t cap;
        uint32_t unused[7];
        uint32_t mode;
        uint32_t evt;
        uint32_t msk;
        uint32_t cmd;
        uint32_t tx;
        uint32_t rx;
};


/* save the state for a given chip/device */
struct grspi_chip {

	void     (*get_rx)(uint32_t rx_data, struct grspi *);
	uint32_t (*get_tx)(struct grspi *);

	uint32_t rx_shift;
	uint32_t tx_shift;

	uint32_t mode_reg;
};

struct grspi {

	struct grspi_regs *regs;
	struct grspi_chip chip;

	const void *tx;	/* spi transfer buffers */
	void *rx;

	uint8_t irq;
	uint8_t count;
	uint8_t word_nbits_max;
	uint8_t n_chipsel;
};


/* SPI controller capability register */
#define GRSPI_CAP_SSEN(r)	(((r) >> 16) & 0x1)
#define GRSPI_CAP_SSSZ(r)	(((r) >> 24) & 0xFF)
#define GRSPI_CAP_MAXWLEN(r)	(((r) >> 20) & 0xF)
#define GRSPI_CAP_FDEPTH(r)	(((r) >>  8) & 0xFF)

/* SPI controller mode register */
#define GRSPI_MODE_LOOP		(1 << 30)		/* enable loopback */
#define GRSPI_MODE_CPOL		(1 << 29)		/* clock polarity in inactive (idle) state: (1:high, 0:low) */
#define GRSPI_MODE_CPHA		(1 << 28)		/* read clock phase; (0: rising edge, 1: falilng edge) */
#define GRSPI_MODE_DIV16	(1 << 27)		/* enable division of sysclk by 16 */
#define GRSPI_MODE_REV		(1 << 26)		/* data order; 0: (LSB first, 1: MSB first) */
#define GRSPI_MODE_MS		(1 << 25)		/* controller mode (0: slave, 1: master) */
#define GRSPI_MODE_EN		(1 << 24)		/* core enable (0: off, 1: on) */
#define GRSPI_MODE_LEN(x)	(((x) & 0xF) << 20)	/* transfer word lenght (0: 32 bit; otherwise nbits == (x +1), valid range x: 3-15) */
#define GRSPI_MODE_PM(x)	(((x) & 0xF) << 16)	/* prescale modulus; see documentation for details */
#define GRSPI_MODE_FACT		(1 << 13)		/* set PM scaling behaviour; if 1, MPC83xx register compatibility is lost */
#define GRSPI_MODE_CG		(((x) & 0x1F) << 7)	/* set number of clock gap cycles between consecutive word transfers */

/* SPI controller event register */
#define GRSPI_EVT_TIP		(1 << 31)	/* transfer in progress, RO */
#define GRSPI_EVT_LT		(1 << 14)	/* last tx, queue empty, clear with 1 */
#define GRSPI_EVT_OV		(1 << 12)	/* rx overrun, queue full, clear with 1 */
#define GRSPI_EVT_UN		(1 << 11)	/* tx underrun, queue empty; slave mode only, clear with 1 */
#define GRSPI_EVT_NE		(1 <<  9)	/* RX not empty */
#define GRSPI_EVT_NF		(1 <<  8)	/* TX not full */

/* SPI interrupt mask register  */
#define GRSPI_MSK_TIPE		(1 << 31)	/* IRQ when EVT_TIP goes high */
#define GRSPI_MSK_LTE		(1 << 14)	/* IRQ when EVT_LT goes high */
#define GRSPI_MSK_OVE		(1 << 12)	/* IRQ when EVT_OV goes high */
#define GRSPI_MSK_UNE		(1 << 11)	/* IRQ when EVT_UN goes high */
#define GRSPI_MSK_MMEE		(1 << 10)	/* IRQ on multiple-master error */
#define GRSPI_MSK_NEE		(1 <<  9)	/* IRQ when EVT_NE goes high */
#define GRSPI_MSK_NFE		(1 <<  8)	/* IRQ when EVT_NF goes high */

/* defaults: active-high polarity, minimum clock (div16), MSB first, master (only supported mode), 8 bit transfers, minimum clock (max prescaler) */
#define	GRSPI_INIT_MODE (GRSPI_MODE_CPOL | GRSPI_MODE_DIV16 | GRSPI_MODE_REV | GRSPI_MODE_MS | GRSPI_MODE_LEN(7) | GRSPI_MODE_PM(15))


#endif /* GRSPI_H */
