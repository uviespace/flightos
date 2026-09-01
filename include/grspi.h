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


/**
 * @brief GRSPI core register map (see GR712RC-UM Mar 2025, v2.17, pp.175)
 */
struct grspi_regs {
        uint32_t cap;		/*!< capability register */
        uint32_t unused[7];
        uint32_t mode;		/*!< mode register */
        uint32_t evt;		/*!< event register */
        uint32_t msk;		/*!< interrupt mask register */
        uint32_t cmd;		/*!< command register */
        uint32_t tx;		/*!< transmit register */
        uint32_t rx;		/*!< receive register */
};


/**
 * @brief saved state for a given chip/device
 */
struct grspi_chip {

	void     (*get_rx)(uint32_t rx_data, struct grspi *);	/*!< RX data callback */
	uint32_t (*get_tx)(struct grspi *);			/*!< TX data callback */

	uint32_t rx_shift;	/*!< RX shift amount */
	uint32_t tx_shift;	/*!< TX shift amount */

	uint32_t mode_reg;	/*!< saved mode register value */
};

/**
 * @brief GRSPI controller state
 */
struct grspi {

	struct grspi_regs *regs;	/*!< register map pointer */
	struct grspi_chip chip;		/*!< chip/device state */

	const void *tx;	/*!< SPI transfer buffers */
	void *rx;	/*!< receive buffer */

	uint8_t irq;		/*!< interrupt line */
	uint8_t count;		/*!< transfer counter */
	uint8_t word_nbits_max;	/*!< maximum word width in bits */
	uint8_t n_chipsel;	/*!< number of chip selects */
};


/* SPI controller capability register */
/** @brief get slave select enable support from the capability register */
#define GRSPI_CAP_SSEN(r)	(((r) >> 16) & 0x1)
/** @brief get the number of slave selects from the capability register */
#define GRSPI_CAP_SSSZ(r)	(((r) >> 24) & 0xFF)
/** @brief get the maximum word length from the capability register */
#define GRSPI_CAP_MAXWLEN(r)	(((r) >> 20) & 0xF)
/** @brief get the FIFO depth from the capability register */
#define GRSPI_CAP_FDEPTH(r)	(((r) >>  8) & 0xFF)

/* SPI controller mode register */
/** @brief enable loopback mode */
#define GRSPI_MODE_LOOP		(1 << 30)
/** @brief clock polarity in inactive (idle) state: (1:high, 0:low) */
#define GRSPI_MODE_CPOL		(1 << 29)
/** @brief read clock phase; (0: rising edge, 1: falling edge) */
#define GRSPI_MODE_CPHA		(1 << 28)
/** @brief enable division of sysclk by 16 */
#define GRSPI_MODE_DIV16	(1 << 27)
/** @brief data order; 0: (LSB first, 1: MSB first) */
#define GRSPI_MODE_REV		(1 << 26)
/** @brief controller mode (0: slave, 1: master) */
#define GRSPI_MODE_MS		(1 << 25)
/** @brief core enable (0: off, 1: on) */
#define GRSPI_MODE_EN		(1 << 24)
/** @brief transfer word length (0: 32 bit; otherwise nbits == (x +1), valid range x: 3-15) */
#define GRSPI_MODE_LEN(x)	(((x) & 0xF) << 20)
/** @brief prescale modulus; see documentation for details */
#define GRSPI_MODE_PM(x)	(((x) & 0xF) << 16)
/** @brief set PM scaling behaviour; if 1, MPC83xx register compatibility is lost */
#define GRSPI_MODE_FACT		(1 << 13)
/** @brief set number of clock gap cycles between consecutive word transfers
 *  @param x the number of clock gap cycles
 */
#define GRSPI_MODE_CG(x)		(((x) & 0x1F) << 7)

/* SPI controller event register */
/** @brief transfer in progress, read-only */
#define GRSPI_EVT_TIP		(1 << 31)
/** @brief last tx, queue empty, clear with 1 */
#define GRSPI_EVT_LT		(1 << 14)
/** @brief rx overrun, queue full, clear with 1 */
#define GRSPI_EVT_OV		(1 << 12)
/** @brief tx underrun, queue empty; slave mode only, clear with 1 */
#define GRSPI_EVT_UN		(1 << 11)
/** @brief RX not empty */
#define GRSPI_EVT_NE		(1 <<  9)
/** @brief TX not full */
#define GRSPI_EVT_NF		(1 <<  8)

/* SPI interrupt mask register  */
/** @brief IRQ when EVT_TIP goes high */
#define GRSPI_MSK_TIPE		(1 << 31)
/** @brief IRQ when EVT_LT goes high */
#define GRSPI_MSK_LTE		(1 << 14)
/** @brief IRQ when EVT_OV goes high */
#define GRSPI_MSK_OVE		(1 << 12)
/** @brief IRQ when EVT_UN goes high */
#define GRSPI_MSK_UNE		(1 << 11)
/** @brief IRQ on multiple-master error */
#define GRSPI_MSK_MMEE		(1 << 10)
/** @brief IRQ when EVT_NE goes high */
#define GRSPI_MSK_NEE		(1 <<  9)
/** @brief IRQ when EVT_NF goes high */
#define GRSPI_MSK_NFE		(1 <<  8)

/** @brief default mode: active-high polarity, minimum clock (div16), MSB first, master, 8-bit transfers, minimum clock (max prescaler) */
#define	GRSPI_INIT_MODE (GRSPI_MODE_CPOL | GRSPI_MODE_DIV16 | GRSPI_MODE_REV | GRSPI_MODE_MS | GRSPI_MODE_LEN(7) | GRSPI_MODE_PM(15))


#endif /* GRSPI_H */
