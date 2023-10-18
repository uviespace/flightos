/**
 * @file   smile_fee.h
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   2020
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
 * @brief SMILE FEE interface
 *
 * @warn byte order is BIG ENDIAN!
 */

#ifndef SMILE_FEE_H
#define SMILE_FEE_H

#include <stdint.h>



/**
 *  @see MSSL-SMILE-SXI-IRD-0001
 */

#define DPU_LOGICAL_ADDRESS	0x50
#define FEE_LOGICAL_ADDRESS	0x51

#define RMAP_PROTOCOL_ID	0x01
#define FEE_DATA_PROTOCOL	0xF0	/* MSSL-IF-95 */



/**
 * FEE modes
 *
 * @see MSSL-SMILE-SXI-IRD-0001, req. MSSL-IF-17
 */


#define FEE_MODE_ID_ON		0x0	/* the thing is switched on */
#define FEE_MODE_ID_FTP		0x1	/* frame transfer pattern */
#define FEE_MODE_ID_STBY	0x2	/* stand-by-mode */
#define FEE_MODE_ID_FT		0x3	/* frame transfer */
#define FEE_MODE_ID_FF		0x4	/* full frame */
#define FEE_MODE_ID_PTP1	0x5	/* parallel trap pump mode 1 */
#define FEE_MODE_ID_PTP2	0x6	/* parallel trap pump mode 2 */
#define FEE_MODE_ID_STP1	0x7	/* serial trap pump mode 1 */
#define FEE_MODE_ID_STP2	0x8	/* serial trap pump mode 2 */


/* @see MSSL-SMILE-SXI-IRD-0001, req. MSSL-IF-97 */

#define FEE_CCD_SIDE_LEFT	0x0	/* side F */
#define FEE_CCD_SIDE_RIGHT	0x1	/* side E */
#define FEE_CCD_INTERLEAVED	0x2	/* F and E inverleaved */

#define FEE_CDD_ID_2		0x0
#define FEE_CDD_ID_4		0x1

#define FEE_PKT_TYPE_DATA	0x0	/* any data */
#define FEE_PKT_TYPE_EV_DET	0x1	/* event detection */
#define FEE_PKT_TYPE_HK		0x2	/* housekeeping */



/* @see MSSL-SMILE-SXI-IRD-0001, req. MSSL-IF-97 */
struct fee_pkt_type {
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
	uint16_t reserved0:4;
	uint16_t fee_mode:4;
	uint16_t last_pkt:1;	/* if set: last packet in readout cycle */
	uint16_t ccd_side:2;
	uint16_t ccd_id:1;
	uint16_t reserved1:2;
	uint16_t pkt_type:2;
#elif (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
	uint16_t pkt_type:2;
	uint16_t reserved1:2;
	uint16_t ccd_id:1;
	uint16_t ccd_side:2;
	uint16_t last_pkt:1;	/* if set: last packet in readout cycle */
	uint16_t fee_mode:4;
	uint16_t reserved0:4;
#endif
} __attribute__((packed));

/* XXX need proper include file in path for compile time size checks */
#if 0
compile_time_assert((sizeof(struct fee_data_hdr)
                     == sizeof(uint16_t)),
                    FEE_PKT_TYPE_STRUCT_WRONG_SIZE);
#endif


/* @see MSSL-SMILE-SXI-IRD-0001, req. MSSL-IF-91 */
__extension__
struct fee_data_hdr {
	uint8_t logical_addr;
	uint8_t proto_id;
	uint16_t data_len;
	union {
		uint16_t fee_pkt_type;
		struct fee_pkt_type type;
	};
	uint16_t frame_cntr;
	uint16_t seq_cntr;
} __attribute__((packed));


/* @see MSSL-SMILE-SXI-IRD-0001, req. MSSL-IF-91 */
#define FEE_EV_COLS		 5
#define FEE_EV_ROWS		 5
#define FEE_EV_DET_PIXELS	25	/* 5x5 grid around event pixel */
#define FEE_EV_PIXEL_IDX	12	/* the index of the event pixel */

__extension__
struct fee_event_dection {
	struct fee_data_hdr hdr;

	uint16_t col;
	uint16_t row;

	uint16_t pix[FEE_EV_DET_PIXELS];

} __attribute__((packed));








#endif /* SMILE_FEE_H */
