/**
 * @file   grspw2.h
 * @ingroup grspw2
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   2015
 *
 * @brief Public configuration, descriptor, packet, routing, and status API for
 *        the GRSPW2 driver.
 *
 * The implementation is in `kernel/grspw2.c`. The fixed core addresses and IRQ
 * values in this header describe the currently supported GR712 mapping; their
 * applicability to other targets needs review.
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
 * @brief core addresses and IRQs in the GR712
 * (does not actually belong here...)
 */

/** @brief base address of SpaceWire core 0 */
#define GRSPW2_BASE_CORE_0	0x80100800
/** @brief base address of SpaceWire core 1 */
#define GRSPW2_BASE_CORE_1	0x80100900
/** @brief base address of SpaceWire core 2 */
#define GRSPW2_BASE_CORE_2      0x80100A00
/** @brief base address of SpaceWire core 3 */
#define GRSPW2_BASE_CORE_3      0x80100B00
/** @brief base address of SpaceWire core 4 */
#define GRSPW2_BASE_CORE_4      0x80100C00
/** @brief base address of SpaceWire core 5 */
#define GRSPW2_BASE_CORE_5      0x80100D00

/** @brief IRQ number of SpaceWire core 0 */
#define GRSPW2_IRQ_CORE0		22
/** @brief IRQ number of SpaceWire core 1 */
#define GRSPW2_IRQ_CORE1		23
/** @brief IRQ number of SpaceWire core 2 */
#define GRSPW2_IRQ_CORE2		24
/** @brief IRQ number of SpaceWire core 3 */
#define GRSPW2_IRQ_CORE3		25
/** @brief IRQ number of SpaceWire core 4 */
#define GRSPW2_IRQ_CORE4		26
/** @brief IRQ number of SpaceWire core 5 */
#define GRSPW2_IRQ_CORE5		27


/* default setting for maximum transfer unit (4 hdr bytes + 1 kiB payload) */
/** @brief default maximum transfer unit (4 hdr bytes + 1 kiB payload) */
#define GRSPW2_DEFAULT_MTU	0x0000404

/* maximum transfer unit hardware limitation (yes, it's a tautology) */
/** @brief maximum transfer unit hardware limitation */
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
 * @brief GRSPW2 RX descriptor control bits
 * see GR712RC-UM, p. 112
 */

#define GRSPW2_RX_DESC_PKTLEN_MASK	0x01FFFFFF	/*!< packet length mask */
/* descriptor is enabled */
#define GRSPW2_RX_DESC_EN		0x02000000	/*!< descriptor is enabled */
/* wrap back to start of table */
#define GRSPW2_RX_DESC_WR		0x04000000	/*!< wrap back to start of table */
/* packet interrupt enable */
#define GRSPW2_RX_DESC_IE		0x08000000	/*!< packet interrupt enable */
/* packet ended with error EOP */
#define GRSPW2_RX_DESC_EP		0x10000000	/*!< packet ended with error EOP */
/* header CRC error detected */
#define GRSPW2_RX_DESC_HC		0x20000000	/*!< header CRC error detected */
/* data CRC error detected */
#define GRSPW2_RX_DESC_DC		0x40000000	/*!< data CRC error detected */
/* Packet was truncated	*/
#define GRSPW2_RX_DESC_TR		0x80000000	/*!< packet was truncated */


/**
 * @brief GRSPW2 TX descriptor control bits
 * see GR712RC-UM, p. 115
 * NOTE: incomplete
 */


/* descriptor is enabled       */
#define GRSPW2_TX_DESC_EN	0x00001000	/*!< descriptor is enabled */
/* wrap back to start of table */
#define GRSPW2_TX_DESC_WR	0x00002000	/*!< wrap back to start of table */
/* packet interrupt enabled    */
#define GRSPW2_TX_DESC_IE	0x00004000	/*!< packet interrupt enabled */



/**
 * @brief GRSPW2 register map, repeating DMA Channels 1-4 are in separate struct
 * see GR712RC-UM, p. 125
 */

struct grspw2_dma_regs {
	uint32_t ctrl_status;			/*!< DMA channel control/status register */
	uint32_t rx_max_pkt_len;		/*!< maximum RX packet length */
	union {
		struct {
			uint32_t tx_desc_base_addr:22;	/*!< TX descriptor table base */
			uint32_t tx_desc_sel:6;		/*!< TX descriptor table select */
			uint32_t reserved0:4;
		};
		uint32_t tx_desc_table_addr;	/*!< TX descriptor table address */
	};
	union {
		struct {
			uint32_t rx_desc_base_addr:22;	/*!< RX descriptor table base */
			uint32_t rx_desc_sel:7;		/*!< RX descriptor table select */
			uint32_t reserved1:3;
		};
		uint32_t rx_desc_table_addr;	/*!< RX descriptor table address */
	};
	uint32_t addr;				/*!< DMA address register */
	uint32_t dummy[3];
};

/**
 * @brief GRSPW2 device register map
 */
struct grspw2_regs {
	uint32_t ctrl;			/*!< 0x00 */
	uint32_t status;                /*!< 0x04 */
	uint32_t nodeaddr;              /*!< 0x08 */
	uint32_t clkdiv;                /*!< 0x0C */
	uint32_t destkey;               /*!< 0x10 */
	uint32_t time;                  /*!< 0x14 */
	uint32_t dummy[2];              /*!< 0x18 - 0x1C */

	struct grspw2_dma_regs dma[4];  /*!< 0x20 - 0x9C */
};


/**
 * @brief GRSPW2 RX descriptor word layout, see GR712-UM, p. 112
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
 * @brief GRSPW2 TX descriptor word layout, see GR712-UM, pp. 115
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
 * @brief an element of the RX descriptor ring, tracked in a doubly linked list
 */
struct grspw2_rx_desc_ring_elem {
	struct grspw2_rx_desc	*desc;	/*!< the RX descriptor */
	struct list_head	 node;	/*!< node in the descriptor list */
};


/**
 * @brief an element of the TX descriptor ring, tracked in a doubly linked list
 */
struct grspw2_tx_desc_ring_elem {
	struct grspw2_tx_desc	*desc;	/*!< the TX descriptor */
	struct list_head	 node;	/*!< node in the descriptor list */
};



/**
 * @brief GRSPW2 core configuration structure
 *
 * @note since we are not able to malloc(), it's easiest to create our lists on
 *       the stack
 */

struct grspw2_core_cfg {

	/** NOTE: actual memory buffers we use could be referenced here */

	/* points to the register map of a grspw2 core */
	struct grspw2_regs *regs;	/*!< register map of the core */

	/* the core's interrupt */
	uint32_t core_irq;		/*!< the core's interrupt */

	/* the ahb interrupt */
	uint32_t ahb_irq;		/*!< the AHB interrupt */

	uint32_t strip_hdr_bytes; /*!< bytes to strip from the RX packets */

	uint32_t max_hdr_size;	/*!< maximum size of tx header */

	uint8_t hdr_proto_id_byte;	/*!< position of protocol ID byte in header */
	uint8_t hdr_proto_id;		/*!< value of protocol ID */
	int inv_proto_drop;		/*!< drop packets with mismatching protocol */

	uint32_t rx_bytes;		/*!< total received bytes */
	uint32_t tx_bytes;		/*!< total transmitted bytes */

	/* irq-driven packet drop mode */
	int auto_drop;			/*!< packet drop mode enabled */
	int n_drop;			/*!< number of packets to drop */
	uint8_t idx_drop;		/*!< drop index */

	struct sysobj sobj;		/*!< sysctl object */

	/* routing node, we currently support only one device and only
	 * blind routing (i.e. address bytes are ignored */
	struct grspw2_core_cfg *route[1];	/*!< routing node */

	/**
	 * the rx and tx descriptor pointers in these arrays must point to the
	 * descriptors in the same order as they are used by the grspw2 core so
	 * they may be sequentially accessed at any time
	 */
	struct grspw2_rx_desc_ring_elem	rx_desc_ring[GRSPW2_RX_DESCRIPTORS];	/*!< RX descriptor ring */
	struct grspw2_tx_desc_ring_elem	tx_desc_ring[GRSPW2_TX_DESCRIPTORS];	/*!< TX descriptor ring */

	/* number of rx/tx descriptors configured */
	uint32_t rx_n_desc;	/*!< number of RX descriptors configured */
	uint32_t tx_n_desc;	/*!< number of TX descriptors configured */

	/**
	 * we use two list heads for each descriptor type to manage active and
	 * inactive descriptors
	 * spin-lock protection is fine as long as the lists are only modified
	 * outside of an ISR or if the ISR may schedule itself to be
	 * re-executed at a later time when the lock has been released
	 */
	struct list_head		rx_desc_ring_used;	/*!< used RX descriptors list */
	struct list_head		rx_desc_ring_free;	/*!< free RX descriptors list */

	struct list_head		tx_desc_ring_used;	/*!< used TX descriptors list */
	struct list_head		tx_desc_ring_free;	/*!< free TX descriptors list */

	/**
	 * @brief allocated descriptor tables and buffers
	 */
	struct  {
		uint32_t *rx_desc_tbl;	/*!< RX descriptor table */
		uint32_t *tx_desc_tbl;	/*!< TX descriptor table */
		uint8_t *rx_descs;	/*!< RX descriptor memory */
		uint8_t *tx_descs;	/*!< TX descriptor memory */
		uint8_t *tx_hdr;	/*!< TX header buffer */
		uint32_t tx_hdr_size;	/*!< TX header buffer size */
	} alloc;

};

/**
 * @brief set the destination key register
 * @param regs: the GRSPW2 register map
 * @param destkey: the destination key value
 */
void grspw2_set_dest_key(struct grspw2_regs *regs, uint8_t destkey);

/**
 * @brief enable RMAP routing on a core
 * @param cfg: the core configuration
 */
void grspw2_set_rmap(struct grspw2_core_cfg *cfg);

/**
 * @brief disable RMAP routing on a core
 * @param cfg: the core configuration
 */
void grspw2_clear_rmap(struct grspw2_core_cfg *cfg);

/**
 * @brief enable promiscuous mode on a core
 * @param cfg: the core configuration
 */
void grspw2_set_promiscuous(struct grspw2_core_cfg *cfg);

/**
 * @brief disable promiscuous mode on a core
 * @param cfg: the core configuration
 */
void grspw2_unset_promiscuous(struct grspw2_core_cfg *cfg);

/**
 * @brief initialize the TX descriptor table
 * @param cfg: the core configuration
 * @param mem: memory for the descriptor table
 * @param tbl_size: descriptor table size
 * @param hdr_buf: buffer for TX headers
 * @param hdr_size: size of the header buffer
 * @param data_buf: buffer for TX data
 * @param data_size: size of the data buffer
 * @return 0 on success, negative error code on failure
 */
int32_t grspw2_tx_desc_table_init(struct grspw2_core_cfg *cfg,
				  uint32_t *mem,      uint32_t  tbl_size,
				  uint8_t *hdr_buf,  uint32_t  hdr_size,
				  uint8_t *data_buf, uint32_t  data_size);

/**
 * @brief initialize the RX descriptor table
 * @param cfg: the core configuration
 * @param mem: memory for the descriptor table
 * @param tbl_size: descriptor table size
 * @param pkt_buf: buffer for received packets
 * @param pkt_size: size of the packet buffer
 * @return 0 on success, negative error code on failure
 */
int32_t grspw2_rx_desc_table_init(struct grspw2_core_cfg *cfg,
				  uint32_t *mem,     uint32_t  tbl_size,
				  uint8_t  *pkt_buf, uint32_t  pkt_size);


/**
 * @brief get the number of received packets available
 * @param cfg: the core configuration
 * @return number of available RX packets
 */
uint32_t grspw2_get_num_pkts_avail(struct grspw2_core_cfg *cfg);

/**
 * @brief get the number of free TX descriptors available
 * @param cfg: the core configuration
 * @return number of free TX descriptors
 */
uint32_t grspw2_get_num_free_tx_desc_avail(struct grspw2_core_cfg *cfg);

/**
 * @brief get the number of free RX descriptors available
 * @param cfg: the core configuration
 * @return number of free RX descriptors
 */
uint32_t grspw2_get_num_free_rx_desc_avail(struct grspw2_core_cfg *cfg);

/**
 * @brief copy the next received packet into a caller buffer
 * @param cfg: the core configuration
 * @param pkt: destination buffer for the packet
 * @return size of the copied packet
 */
uint32_t grspw2_get_pkt(struct grspw2_core_cfg *cfg, uint8_t *pkt);

/**
 * @brief get a reference to the next received packet
 * @param cfg: the core configuration
 * @param pkt: output pointer to the packet data
 * @return size of the packet
 */
uint32_t grspw2_get_pkt_ref(struct grspw2_core_cfg *cfg, uint8_t **pkt);

/**
 * @brief drop the next received packet
 * @param cfg: the core configuration
 * @return number of packets dropped
 */
uint32_t grspw2_drop_pkt(struct grspw2_core_cfg *cfg);

/**
 * @brief get the size of the next received packet without removing it
 * @param cfg: the core configuration
 * @return size of the next packet
 */
uint32_t grspw2_get_next_pkt_size(struct grspw2_core_cfg *cfg);

/**
 * @brief check if the next packet ended with an early EOP
 * @param cfg: the core configuration
 * @return non-zero if early EOP, 0 otherwise
 */
int grspw2_get_next_pkt_eep(struct grspw2_core_cfg *cfg);

/**
 * @brief enable automatic packet drop mode
 * @param cfg: the core configuration
 * @param n_drop: number of packets to auto-drop
 * @return 0 on success, negative error code on failure
 */
int grspw2_auto_drop_enable(struct grspw2_core_cfg *cfg, uint8_t n_drop);

/**
 * @brief disable automatic packet drop mode
 * @param cfg: the core configuration
 * @return 0 on success, negative error code on failure
 */
int grspw2_auto_drop_disable(struct grspw2_core_cfg *cfg);

/**
 * @brief enable dropping packets with a specific protocol ID
 * @param cfg: the core configuration
 * @param idx: header byte position of the protocol ID
 * @param id: protocol ID value to drop
 */
void grspw2_protocol_id_drop_enable(struct grspw2_core_cfg *cfg, uint8_t idx, uint8_t id);

/**
 * @brief disable protocol ID based packet dropping
 * @param cfg: the core configuration
 */
void grspw2_protocol_id_drop_disable(struct grspw2_core_cfg *cfg);


/**
 * @brief generate a tick-in on the core
 * @param cfg: the core configuration
 */
void grspw2_tick_in(struct grspw2_core_cfg *cfg);

/**
 * @brief get the current time counter value
 * @param cfg: the core configuration
 * @return current time counter value
 */
uint32_t grspw2_get_timecnt(struct grspw2_core_cfg *cfg);

/**
 * @brief get the current link status
 * @param cfg: the core configuration
 * @return link status code
 */
uint32_t grspw2_get_link_status(struct grspw2_core_cfg *cfg);

/**
 * @brief enable the tick-out interrupt
 * @param cfg: the core configuration
 */
void grspw2_tick_out_interrupt_enable(struct grspw2_core_cfg *cfg);

/**
 * @brief configure the core to receive time codes
 * @param cfg: the core configuration
 */
void grspw2_set_time_rx(struct grspw2_core_cfg *cfg);

/**
 * @brief add a packet to the TX buffer
 * @param cfg: the core configuration
 * @param hdr: packet header data
 * @param hdr_size: header size in bytes
 * @param data: packet payload data
 * @param data_size: payload size in bytes
 * @return 0 on success, negative error code on failure
 */
int32_t grspw2_add_pkt(struct grspw2_core_cfg *cfg,
			const void *hdr,  uint32_t hdr_size,
			const void *data, uint32_t data_size);

/**
 * @brief add an RMAP packet to the TX buffer
 * @param cfg: the core configuration
 * @param hdr: packet header data
 * @param hdr_size: header size in bytes
 * @param non_crc_bytes: number of non-CRC bytes in the header
 * @param data: packet payload data
 * @param data_size: payload size in bytes
 * @return 0 on success, negative error code on failure
 */
int32_t grspw2_add_rmap(struct grspw2_core_cfg *cfg,
			const void *hdr,  uint32_t hdr_size,
			const uint8_t non_crc_bytes,
			const void *data, uint32_t data_size);

/**
 * @brief start the SpaceWire core
 * @param cfg: the core configuration
 * @param link_start: whether to start the link
 * @param auto_start: whether to enable link autostart
 */
void grspw2_core_start(struct grspw2_core_cfg *cfg, int link_start, int auto_start);


/**
 * @brief initialize a GRSPW2 core
 * @param cfg: the core configuration to populate
 * @param core_addr: base address of the core
 * @param node_addr: SpaceWire logical node address
 * @param link_start: whether to start the link on init
 * @param link_run: whether to keep the link running
 * @param mtu: maximum transfer unit
 * @param core_irq: interrupt for the core
 * @param ahb_irq: AHB interrupt
 * @param strip_hdr_bytes: bytes to strip from RX packets
 * @return 0 on success, negative error code on failure
 */
int32_t grspw2_core_init(struct grspw2_core_cfg *cfg, uint32_t core_addr,
			 uint8_t node_addr, uint8_t link_start,
			 uint8_t link_run, uint32_t mtu,
			 uint32_t core_irq, uint32_t ahb_irq,
			 uint32_t strip_hdr_bytes);


/**
 * @brief enable blind routing to another core
 * @param cfg: the source core configuration
 * @param route: the destination core configuration
 * @return 0 on success, negative error code on failure
 */
int32_t grspw2_enable_routing(struct grspw2_core_cfg *cfg,
			      struct grspw2_core_cfg *route);

/**
 * @brief enable routing without using the routing ISR
 * @param cfg: the source core configuration
 * @param route: the destination core configuration
 * @return 0 on success, negative error code on failure
 */
int32_t grspw2_enable_routing_noirq(struct grspw2_core_cfg *cfg,
				    struct grspw2_core_cfg *route);

/**
 * @brief route a packet from one core to another (ISR handler)
 * @param userdata: pointer to the source core configuration
 * @return 0 on success, negative error code on failure
 */
int32_t grspw2_route(void *userdata);

/**
 * @brief disable routing on a core
 * @param cfg: the core configuration
 * @return 0 on success, negative error code on failure
 */
int32_t grspw2_disable_routing(struct grspw2_core_cfg *cfg);

/**
 * @brief set the SpaceWire clock on the GR712RC
 */
void set_gr712_spw_clock(void);


/**
 * @brief enable the link error interrupt
 * @param cfg: the core configuration
 */
void grspw2_set_link_error_irq(struct grspw2_core_cfg *cfg);

/**
 * @brief disable the link error interrupt
 * @param cfg: the core configuration
 */
void grspw2_unset_link_error_irq(struct grspw2_core_cfg *cfg);

/**
 * @brief perform a hardware reset of the SpaceWire core
 * @param regs: the core register map
 */
void grspw2_spw_hardreset(struct grspw2_regs *regs);

/**
 * @brief enable the RX interrupt
 * @param cfg: the core configuration
 */
void grspw2_rx_interrupt_enable(struct grspw2_core_cfg *cfg);

/**
 * @brief disable the RX interrupt
 * @param cfg: the core configuration
 */
void grspw2_rx_interrupt_disable(struct grspw2_core_cfg *cfg);



/* XXX have this temporarily for this syscall interface */

#define GRSPW2_OP_ADD_PKT		1	/*!< add a TX packet */
#define GRSPW2_OP_ADD_RMAP		2	/*!< add an RMAP packet */
#define GRSPW2_OP_GET_NUM_PKT_AVAIL	3	/*!< get number of available RX packets */
#define GRSPW2_OP_GET_NEXT_PKT_SIZE	4	/*!< get size of the next RX packet */
#define GRSPW2_OP_DROP_PKT		5	/*!< drop a packet */
#define GRSPW2_OP_GET_PKT		6	/*!< fetch a packet */
#define GRSPW2_OP_GET_NEXT_PKT_EEP	7	/*!< get the EEP of the next RX packet */
#define GRSPW2_OP_AUTO_DROP_ENABLE	8	/*!< enable auto drop */
#define GRSPW2_OP_AUTO_DROP_DISABLE	9	/*!< disable auto drop */
#define GRSPW2_OP_GET_PKT_REF		10	/*!< get a packet reference */


/**
 * @brief a spacewire core configuration
 */
struct spw_user_cfg {
	struct grspw2_core_cfg spw;
	uint32_t *rx_desc;
	uint32_t *tx_desc;
	uint8_t  *rx_data;
	uint8_t  *tx_data;
	uint8_t  *tx_hdr;
};

/**
 * @brief data structure to transfer grspw2 info via syscall
 *
 * @note this is a temporary interface; later this should be migrated to the
 *	 file read/write interface
 */
struct grspw2_data {
	uint8_t op;

	uint8_t link;
	void *hdr;
	uint32_t hdr_size;
	uint8_t non_crc_bytes;
	void *data;
	uint32_t data_size;
	uint8_t *pkt;
	uint8_t n_drop;
};


#endif
