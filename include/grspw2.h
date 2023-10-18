/**
 * @file   grspw2.h
 * @ingroup grspw2
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   2015
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

#ifndef GRSPW2_H
#define GRSPW2_H


#include <list.h>
#include <compiler.h>
#include <kernel/types.h>
#include <kernel/sysctl.h>

/**
 * core addresses and IRQs in the GR712
 * (does not actually belong here...)
 */

#define GRSPW2_BASE_CORE_0	0x80100800
#define GRSPW2_BASE_CORE_1	0x80100900
#define GRSPW2_BASE_CORE_2      0x80100A00
#define GRSPW2_BASE_CORE_3      0x80100B00
#define GRSPW2_BASE_CORE_4      0x80100C00
#define GRSPW2_BASE_CORE_5      0x80100D00

#define GRSPW2_IRQ_CORE0		22
#define GRSPW2_IRQ_CORE1		23
#define GRSPW2_IRQ_CORE2		24
#define GRSPW2_IRQ_CORE3		25
#define GRSPW2_IRQ_CORE4		26
#define GRSPW2_IRQ_CORE5		27


/* default setting for maximum transfer unit (4 hdr bytes + 1 kiB payload) */
#define GRSPW2_DEFAULT_MTU	0x0000404

/* maximum transfer unit hardware limitation (yes, it's a tautology) */
#define GRSPW2_MAX_MTU		0x1FFFFFC



/**
 * GRSPW2 control register bit masks
 * see GR712RC-UM, p. 126
 */


#define GRSPW2_CTRL_LD		0x00000001	/* Link Disable               */
#define GRSPW2_CTRL_LS		0x00000002	/* Link Start                 */
#define GRSPW2_CTRL_AS		0x00000004	/* Autostart                  */
#define GRSPW2_CTRL_IE		0x00000008	/* Interrupt Enable           */
#define GRSPW2_CTRL_TI		0x00000010	/* Tick In                    */
#define GRSPW2_CTRL_PM		0x00000020	/* Promiscuous Mode           */
#define GRSPW2_CTRL_RS		0x00000040	/* Reset                      */
#define GRSPW2_CTRL_DUMMY1	0x00000080	/* bit 7 == unused            */
#define GRSPW2_CTRL_TQ		0x00000100	/* Tick-out IRQ               */
#define GRSPW2_CTRL_LI		0x00000200	/* Link error IRQ             */
#define GRSPW2_CTRL_TT		0x00000400	/* Time Tx Enable             */
#define GRSPW2_CTRL_TR		0x00000800	/* Time Rx Enable             */
#define GRSPW2_CTRL_DUMMY2	0x00001000	/* bit 12 == unused           */
#define GRSPW2_CTRL_DUMMY3	0x00002000	/* bit 13 == unused           */
#define GRSPW2_CTRL_DUMMY4	0x00004000	/* bit 14 == unused           */
#define GRSPW2_CTRL_DUMMY5	0x00008000	/* bit 15 == unused           */
#define GRSPW2_CTRL_RE		0x00010000	/* RMAP Enable                */
#define GRSPW2_CTRL_RD		0x00020000	/* RMAP buffer disable        */
#define GRSPW2_CTRL_DUMMY6	0x00040000	/* bit 18 == unused           */
#define GRSPW2_CTRL_DUMMY7	0x00080000	/* bit 19 == unused           */
#define GRSPW2_CTRL_NP		0x00100000	/* No port force              */
#define GRSPW2_CTRL_PS		0x00200000	/* Port select                */
#define GRSPW2_CTRL_DUMMY8	0x00400000	/* bit 22 == unused           */
#define GRSPW2_CTRL_DUMMY9	0x00800000	/* bit 23 == unused           */
#define GRSPW2_CTRL_DUMMY10	0x01000000	/* bit 24 == unused           */
#define GRSPW2_CTRL_DUMMY11	0x02000000	/* bit 25 == unused           */
#define GRSPW2_CTRL_PO		0x04000000	/* Number of ports - 1        */
#define GRSPW2_CTRL_NCH		0x18000000	/* Number of DMA channels - 1 */
#define GRSPW2_CTRL_RC		0x20000000	/* RMAP CRC available         */
#define GRSPW2_CTRL_RX		0x40000000	/* RX unaligned access        */
#define GRSPW2_CTRL_RA		0x80000000	/* RMAP available             */

#define GRSPW2_CTRL_RX_BIT		30
#define GRSPW2_CTRL_RX_BIT_MASK        0x1

#define GRSPW2_CTRL_NCH_BIT		27
#define GRSPW2_CTRL_NCH_BIT_MASK       0x3

#define GRSPW2_CTRL_PO_BIT		26
#define GRSPW2_CTRL_PO_BIT_MASK        0x1


#define GRSPW2_CTRL_GET_RX(x)	\
	(((x >> GRSPW2_CTRL_RX_BIT)  & GRSPW2_CTRL_RX_BIT_MASK))

#define GRSPW2_CTRL_GET_NCH(x)	\
	(((x >> GRSPW2_CTRL_NCH_BIT) & GRSPW2_CTRL_NCH_BIT_MASK) + 1)

#define GRSPW2_CTRL_GET_PO(x)	\
	(((x >> GRSPW2_CTRL_PO_BIT)  & GRSPW2_CTRL_PO_BIT_MASK)  + 1)


/**
 * GRSPW2 control register bit masks
 * see GR712RC-UM, p. 127
 */


#define GRSPW2_STATUS_TO	0x00000001	/* Tick Out             */
#define GRSPW2_STATUS_CE	0x00000002	/* Credit Error         */
#define GRSPW2_STATUS_ER	0x00000004	/* Escape Error         */
#define GRSPW2_STATUS_DE	0x00000008	/* Disconnect Error     */
#define GRSPW2_STATUS_PE	0x00000010	/* Parity Error         */
#define GRSPW2_STATUS_DUMMY1	0x00000020	/* bit 5 == unused      */
#define GRSPW2_STATUS_DUMMY2	0x00000040	/* bit 6 == unused      */
#define GRSPW2_STATUS_IA	0x00000080	/* Invalid Address      */
#define GRSPW2_STATUS_EE	0x00000100	/* Early EOP/EEP        */
#define GRSPW2_STATUS_AP	0x00000200	/* Active port          */
/* bits 10-20 = unused  */
#define GRSPW2_STATUS_LS	0x00E00000
						/* bits 24-31 == unused */

#define GRSPW2_STATUS_CLEAR_MASK     0x19F	/* TO|CE|ER|DE|PE|IA|EE */
#define GRSPW2_STATUS_LS_BIT		21
#define GRSPW2_STATUS_LS_MASK	       0x7

#define GRSPW2_STATUS_GET_LS(x)	\
	((x >> GRSPW2_STATUS_LS_BIT) & GRSPW2_STATUS_LS_MASK)

#define GRSPW2_STATUS_LS_ERROR_RESET	0x0
#define GRSPW2_STATUS_LS_ERROR_WAIT	0x1
#define GRSPW2_STATUS_LS_READY		0x2
#define GRSPW2_STATUS_LS_STARTED	0x3
#define GRSPW2_STATUS_LS_CONNECTING	0x4
#define GRSPW2_STATUS_LS_RUN		0x5



/**
 * GRSPW2 default address register bit masks
 * see GR712RC-UM, p. 127
 */

#define GRSPW2_DEFAULT_ADDR_DEFADDR_BITS         0x00FF
#define GRSPW2_DEFAULT_ADDR_DEFADDR_RESETVAL        254

#define GRSPW2_DEFAULT_ADDR_DEFMASK_BITS	 0x00FF
#define GRSPW2_DEFAULT_ADDR_DEFMASK              0xFF00


/**
 * GRSPW2 clock divisior register bit masks
 * see GR712RC-UM, p. 127
 */

#define GRSPW2_CLOCKDIV_RUN_MASK		0x00FF
#define GRSPW2_CLOCKDIV_START_MASK		0xFF00
#define GRSPW2_CLOCKDIV_START_BIT		     8


/**
 * GRSPW2 destination key register
 * see GR712RC-UM, p. 128
 */

#define GRSPW2_DESTKEY_MASK			0x00FF


/**
 * GRSPW2 time register
 * see GR712RC-UM, p. 128
 */

#define GRSPW2_TIME_TCTRL_BIT			     6
#define GRSPW2_TIME_TCTRL			0x00C0
#define GRSPW2_TIME_TIMECNT			0x003F


/**
 * GRSPW2 DMA control register
 * see GR712RC-UM, p. 128-129
 */

#define GRSPW2_DMACONTROL_TE	0x00000001	/* Transmitter enable       */
#define GRSPW2_DMACONTROL_RE	0x00000002	/* Receiver enable          */
#define GRSPW2_DMACONTROL_TI	0x00000004	/* Transmit interrupt       */
#define GRSPW2_DMACONTROL_RI	0x00000008	/* Receive interrupt        */
#define GRSPW2_DMACONTROL_AI	0x00000010	/* AHB error interrup       */
#define GRSPW2_DMACONTROL_PS	0x00000020	/* Packet sent              */
#define GRSPW2_DMACONTROL_PR	0x00000040	/* Packet received          */
#define GRSPW2_DMACONTROL_TA	0x00000080	/* TX AHB error             */
#define GRSPW2_DMACONTROL_RA	0x00000100	/* RX AHB error             */
#define GRSPW2_DMACONTROL_AT	0x00000200	/* Abort TX                 */
#define GRSPW2_DMACONTROL_RX	0x00000400	/* RX active                */
#define GRSPW2_DMACONTROL_RD	0x00000800	/* RX descriptors available */
#define GRSPW2_DMACONTROL_NS	0x00001000	/* No spill                 */
#define GRSPW2_DMACONTROL_EN	0x00002000	/* Enable addr              */
#define GRSPW2_DMACONTROL_SA	0x00004000	/* Strip addr               */
#define GRSPW2_DMACONTROL_SP	0x00008000	/* Strip pid                */
#define GRSPW2_DMACONTROL_LE	0x00010000	/* Link error disable       */
						/* bits 17-31 == unused     */

/**
 * GRSPW2 RX maximum length register
 * see GR712RC-UM, p. 129
 */

#define GRSPW2_RX_MAX_LEN_MASK	 0xFFFFFF


/**
 * GRSPW2 transmitter descriptor table address register
 * see GR712RC-UM, p. 129
 */

#define GRSWP2_TX_DESCRIPTOR_TABLE_DESCBASEADDR_BIT			10
#define GRSWP2_TX_DESCRIPTOR_TABLE_DESCBASEADDR_REG_MASK	0xFFFFFC00
#define GRSWP2_TX_DESCRIPTOR_TABLE_DESCBASEADDR_BIT_MASK	  0xFFFFFC

#define GRSWP2_TX_DESCRIPTOR_TABLE_DESCSEL_BIT				 4
#define GRSPW2_TX_DESCRIPTOR_TABLE_DESCSEL_REG_MASK		     0x3F0
#define GRSPW2_TX_DESCRIPTOR_TABLE_DESCSEL_BIT_MASK		      0x3F

#define GRSPW2_TX_DESCRIPTOR_TABLE_GET_DESCSEL(x)	\
	(((x) >> GRSWP2_TX_DESCRIPTOR_TABLE_DESCSEL_BIT)\
	 & GRSPW2_TX_DESCRIPTOR_TABLE_DESCSEL_BIT_MASK)


/**
 * GRSPW2 receiver descriptor table address register
 * see GR712RC-UM, p. 129
 */

#define GRSWP2_RX_DESCRIPTOR_TABLE_DESCBASEADDR_BIT			10
#define GRSWP2_RX_DESCRIPTOR_TABLE_DESCBASEADDR_REG_MASK	0xFFFFFC00
#define GRSWP2_RX_DESCRIPTOR_TABLE_DESCBASEADDR_BIT_MASK	  0xFFFFFC

#define GRSWP2_RX_DESCRIPTOR_TABLE_DESCSEL_BIT				 4
#define GRSPW2_RX_DESCRIPTOR_TABLE_DESCSEL_REG_MASK		     0x3F0
#define GRSPW2_RX_DESCRIPTOR_TABLE_DESCSEL_BIT_MASK		      0x3F

#define GRSPW2_RX_DESCRIPTOR_TABLE_GET_DESCSEL(x)	\
	(((x) >> GRSWP2_RX_DESCRIPTOR_TABLE_DESCSEL_BIT)\
	 & GRSPW2_RX_DESCRIPTOR_TABLE_DESCSEL_BIT_MASK)


/**
 * GRSPW2 dma channel address register
 * see GR712RC-UM, p. 129
 */

#define GRSPW2_DMA_CHANNEL_MASK_BIT		     8
#define GRSPW2_DMA_CHANNEL_MASK_BIT_MASK	0x00FF
#define GRSPW2_DMA_CHANNEL_MASK_REG_MASK	0xFF00

#define GRSPW2_DMA_CHANNEL_ADDR_REG_MASK	0x00FF





/* Maximum number of TX Descriptors */
#define GRSPW2_TX_DESCRIPTORS				   64

/* Maximum number of RX Descriptors */
#define GRSPW2_RX_DESCRIPTORS				  128

#define GRSPW2_RX_DESC_SIZE				    8
#define GRSPW2_TX_DESC_SIZE				   16

/* BD Table Size (RX or TX) */
#define GRSPW2_DESCRIPTOR_TABLE_SIZE			0x400

/* alignment of a descriptor table (1024 bytes) */
#define GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN		0x3FF

/**
 * GRSPW2 RX descriptor control bits
 * see GR712RC-UM, p. 112
 */

#define GRSPW2_RX_DESC_PKTLEN_MASK	0x01FFFFFF
/* descriptor is enabled */
#define GRSPW2_RX_DESC_EN		0x02000000
/* wrap back to start of table */
#define GRSPW2_RX_DESC_WR		0x04000000
/* packet interrupt enable */
#define GRSPW2_RX_DESC_IE		0x08000000
/* packet ended with error EOP */
#define GRSPW2_RX_DESC_EP		0x10000000
/* header CRC error detected */
#define GRSPW2_RX_DESC_HC		0x20000000
/* data CRC error detected */
#define GRSPW2_RX_DESC_DC		0x40000000
/* Packet was truncated	*/
#define GRSPW2_RX_DESC_TR		0x80000000


/**
 * GRSPW2 TX descriptor control bits
 * see GR712RC-UM, p. 115
 * NOTE: incomplete
 */


/* descriptor is enabled       */
#define GRSPW2_TX_DESC_EN	0x00001000
/* wrap back to start of table */
#define GRSPW2_TX_DESC_WR	0x00002000
/* packet interrupt enabled    */
#define GRSPW2_TX_DESC_IE	0x00004000



/**
 *	GRSPW2 register map, repeating DMA Channels 1-4 are in separate struct
 *	see GR712RC-UM, p. 125
 */

struct grspw2_dma_regs {
	uint32_t ctrl_status;
	uint32_t rx_max_pkt_len;
	uint32_t tx_desc_table_addr;
	uint32_t rx_desc_table_addr;
	uint32_t addr;
	uint32_t dummy[3];
};

struct grspw2_regs {
	uint32_t ctrl;			/* 0x00 */
	uint32_t status;                /* 0x04 */
	uint32_t nodeaddr;              /* 0x08 */
	uint32_t clkdiv;                /* 0x0C */
	uint32_t destkey;               /* 0x10 */
	uint32_t time;                  /* 0x14 */
	uint32_t dummy[2];              /* 0x18 - 0x1C */

	struct grspw2_dma_regs dma[4];  /* 0x20 - 0x9C */
};


/**
 * GRSPW2 RX descriptor word layout, see GR712-UM, p. 112
 */

__extension__
struct grspw2_rx_desc {
	union {
		struct {
			uint32_t truncated       : 1;
			uint32_t crc_error_data  : 1;
			uint32_t crc_error_header: 1;
			uint32_t EEP_termination : 1;
			uint32_t interrupt_enable: 1;
			uint32_t wrap            : 1;
			uint32_t enable          : 1;
			uint32_t pkt_size        :25;
		};
		uint32_t pkt_ctrl;
	};

	uint32_t pkt_addr;
};



/**
 * check whether the descriptor structure was actually aligned to be the same
 * size as a rx descriptor block as used by the grspw2 core
 */
compile_time_assert((sizeof(struct grspw2_rx_desc) == GRSPW2_RX_DESC_SIZE),
		    RXDESC_STRUCT_WRONG_SIZE);

/**
 * GRSPW2 TX descriptor word layout, see GR712-UM, pp. 115
 */
__extension__
struct grspw2_tx_desc {
	union {
		struct {
			uint32_t reserved1        :14;
			uint32_t append_data_crc  : 1;
			uint32_t append_header_crc: 1;
			uint32_t link_error       : 1;
			uint32_t interrupt_enable : 1;
			uint32_t wrap             : 1;
			uint32_t enable           : 1;
			uint32_t non_crc_bytes    : 4;
			uint32_t hdr_size         : 8;
		};
		uint32_t pkt_ctrl;
	};

	uint32_t hdr_addr;

	union {
		struct {
			uint32_t reserved2      : 8;
			uint32_t data_size      :24;
		};
		uint32_t data_size_reg;
	};

	uint32_t data_addr;
};

/**
 * check whether the descriptor structure was actually aligned to be the same
 * size as a tx descriptor block as used by the grspw2 core
 */
compile_time_assert((sizeof(struct grspw2_tx_desc) == GRSPW2_TX_DESC_SIZE),
		    TXDESC_STRUCT_WRONG_SIZE);


/**
 * the descriptor ring elements are tracked in a doubly linked list
 */


struct grspw2_rx_desc_ring_elem {
	struct grspw2_rx_desc	*desc;
	struct list_head	 node;
};


struct grspw2_tx_desc_ring_elem {
	struct grspw2_tx_desc	*desc;
	struct list_head	 node;
};



/**
 * grspw2 core configuration structure
 * since we are not able to malloc(), it's easiest to create our lists on
 * the stack
 */

struct grspw2_core_cfg {

	/** NOTE: actual memory buffers we use could be referenced here */

	/* points to the register map of a grspw2 core */
	struct grspw2_regs *regs;

	/* the core's interrupt */
	uint32_t core_irq;

	/* the ahb interrupt */
	uint32_t ahb_irq;

	uint32_t strip_hdr_bytes; /* bytes to strip from the RX packets */

	uint32_t rx_bytes;
	uint32_t tx_bytes;

	struct sysobj sobj;

	/* routing node, we currently support only one device and only
	 * blind routing (i.e. address bytes are ignored */
	struct grspw2_core_cfg *route[1];

	/**
	 * the rx and tx descriptor pointers in these arrays must point to the
	 * descriptors in the same order as they are used by the grspw2 core so
	 * they may be sequentially accessed at any time
	 */
	struct grspw2_rx_desc_ring_elem	rx_desc_ring[GRSPW2_RX_DESCRIPTORS];
	struct grspw2_tx_desc_ring_elem	tx_desc_ring[GRSPW2_TX_DESCRIPTORS];

	/* number of rx/tx descriptors configured */
	uint32_t rx_n_desc;
	uint32_t tx_n_desc;

	/**
	 * we use two list heads for each descriptor type to manage active and
	 * inactive descriptors
	 * spin-lock protection is fine as long as the lists are only modified
	 * outside of an ISR or if the ISR may schedule itself to be
	 * re-executed at a later time when the lock has been released
	 */
	struct list_head		rx_desc_ring_used;
	struct list_head		rx_desc_ring_free;

	struct list_head		tx_desc_ring_used;
	struct list_head		tx_desc_ring_free;

	struct  {
		uint32_t *rx_desc_tbl;
		uint32_t *tx_desc_tbl;
		uint8_t *rx_descs;
		uint8_t *tx_descs;
		uint8_t *tx_hdr;
		uint32_t tx_hdr_size;
	} alloc;

};

void grspw2_set_dest_key(struct grspw2_regs *regs, uint8_t destkey);

void grspw2_set_rmap(struct grspw2_core_cfg *cfg);
void grspw2_clear_rmap(struct grspw2_core_cfg *cfg);

void grspw2_set_promiscuous(struct grspw2_core_cfg *cfg);
void grspw2_unset_promiscuous(struct grspw2_core_cfg *cfg);

int32_t grspw2_tx_desc_table_init(struct grspw2_core_cfg *cfg,
				  uint32_t *mem,      uint32_t  tbl_size,
				  uint8_t *hdr_buf,  uint32_t  hdr_size,
				  uint8_t *data_buf, uint32_t  data_size);

int32_t grspw2_rx_desc_table_init(struct grspw2_core_cfg *cfg,
				  uint32_t *mem,     uint32_t  tbl_size,
				  uint8_t  *pkt_buf, uint32_t  pkt_size);


uint32_t grspw2_get_num_pkts_avail(struct grspw2_core_cfg *cfg);
uint32_t grspw2_get_num_free_tx_desc_avail(struct grspw2_core_cfg *cfg);
uint32_t grspw2_get_num_free_rx_desc_avail(struct grspw2_core_cfg *cfg);

uint32_t grspw2_get_pkt(struct grspw2_core_cfg *cfg, uint8_t *pkt);
uint32_t grspw2_drop_pkt(struct grspw2_core_cfg *cfg);
uint32_t grspw2_get_next_pkt_size(struct grspw2_core_cfg *cfg);

void grspw2_tick_in(struct grspw2_core_cfg *cfg);
uint32_t grspw2_get_timecnt(struct grspw2_core_cfg *cfg);
uint32_t grspw2_get_link_status(struct grspw2_core_cfg *cfg);

void grspw2_tick_out_interrupt_enable(struct grspw2_core_cfg *cfg);
void grspw2_set_time_rx(struct grspw2_core_cfg *cfg);

int32_t grspw2_add_pkt(struct grspw2_core_cfg *cfg,
			const void *hdr,  uint32_t hdr_size,
			const void *data, uint32_t data_size);

int32_t grspw2_add_rmap(struct grspw2_core_cfg *cfg,
			const void *hdr,  uint32_t hdr_size,
			const uint8_t non_crc_bytes,
			const void *data, uint32_t data_size);

void grspw2_core_start(struct grspw2_core_cfg *cfg, int link_start, int auto_start);


int32_t grspw2_core_init(struct grspw2_core_cfg *cfg, uint32_t core_addr,
			 uint8_t node_addr, uint8_t link_start,
			 uint8_t link_run, uint32_t mtu,
			 uint32_t core_irq, uint32_t ahb_irq,
			 uint32_t strip_hdr_bytes);


int32_t grspw2_enable_routing(struct grspw2_core_cfg *cfg,
			      struct grspw2_core_cfg *route);

int32_t grspw2_enable_routing_noirq(struct grspw2_core_cfg *cfg,
				    struct grspw2_core_cfg *route);
int32_t grspw2_route(void *userdata);

int32_t grspw2_disable_routing(struct grspw2_core_cfg *cfg);

void set_gr712_spw_clock(void);


void grspw2_set_link_error_irq(struct grspw2_core_cfg *cfg);
void grspw2_unset_link_error_irq(struct grspw2_core_cfg *cfg);
void grspw2_spw_hardreset(struct grspw2_regs *regs);




/* XXX have this temporarily for this syscall interface */

#define GRSPW2_OP_ADD_PKT		1
#define GRSPW2_OP_ADD_RMAP		2
#define GRSPW2_OP_GET_NUM_PKT_AVAIL	3
#define GRSPW2_OP_GET_NEXT_PKT_SIZE	4
#define GRSPW2_OP_DROP_PKT		5
#define GRSPW2_OP_GET_PKT		6

/* a spacewire core configuration */
struct spw_user_cfg {
	struct grspw2_core_cfg spw;
	uint32_t *rx_desc;
	uint32_t *tx_desc;
	uint8_t  *rx_data;
	uint8_t  *tx_data;
	uint8_t  *tx_hdr;
};
/* XXX to transfer info via syscall; later we should migrate this to the file r/w interface */
struct grspw2_data {
	uint8_t op;

	uint8_t link;
	void *hdr;
	uint32_t hdr_size;
	uint8_t non_crc_bytes;
	void *data;
	uint32_t data_size;
	uint8_t *pkt;
};


#endif
