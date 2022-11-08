/**
 * @file   grspw2.c
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
 * @defgroup grspw2 GRSPW2 driver
 * @brief Driver for the GRSPW2 IP core
 *
 * ## Overview
 *
 * This is a driver for the GRSPW2 IP core that operates without interrupts
 * in normal mode and with internally managed interrupts in packet routing mode.
 * It maintains its own ring-buffer-like tables of transfer descriptors that are
 * used by the GRSPW2 core. These can store up to 64 TX and 128 RX packets at a
 * time.
 *
 * ## Mode of Operation
 *
 * @see _GR712RC user manual v2.7 chapter 16_ for information on the SpW IP core
 *
 * The transfer descriptors are managed by tracking their usage status in
 * doubly-linked lists and moving their references between a free and a busy
 * list.
 *
 * @startuml{grspw2_add_pkt.svg} "add TX packet" width=10
 *	actor User
 *	database Free
 *	database Busy
 *	entity SpW
 *
 *	Free <- Busy		: free
 *	note left
 *		processed descriptors
 *		are moved to free list
 *	end note
 *
 *	User <- Free		: retrieve
 *	User -> Busy		: activate
 *
 *	Busy --> SpW		: send
 *	note left
 *		SpW core is notified
 *		about new descriptors
 *	end note
 * @enduml
 *
 *
 * TX packets are sent (and hence freed) as fast as possible by the core.
 * RX packets are accumulated by the device until all unused descriptors
 * are filled up, after which the core will no longer accept packets on its link
 * interface, until at least one stored entry is fetched by the user. Because of
 * how the SpaceWire protocol works, no packet loss will occur on the link
 * level. Transfers just pause until continued.
 *
 * @startuml{grspw2_get_pkt.svg} "get RX packet" width=10
 *	actor User
 *	database Free
 *	database Busy
 *	entity SpW
 *
 *	Busy <- SpW		: receive
 *
 *	note right
 *		used descriptors are
 *		marked by the core
 *	end note
 *
 *	hnote over SpW: ...
 *
 *	User <-- Busy		: check status
 *
 *	hnote over User: ...
 *
 *	User <-  Busy		: fetch
 *	User ->  Free		: release
 *	Free --> Busy		: reactivate
 *
 *	note left
 *		during typical
 *		operation
 *	end note
 *
 *
 *
 *
 * @enduml
 *
 * While not strictly necessary, RX descriptors are tracked in a free/busy list
 * as well to allow the same level of control as with TX descriptors.
 *
 *
 * ### Routing mode
 *
 * There is support for direct routing of packets between SpW cores. Since
 * immediate re-transmission is required for low-latency, low packet rate
 * configurations, the routing is by default set up to generate one interrupt
 * per received packet. For alternative applications with either constant high
 * packet rates or high latency tolerance, the routing could be reconfigured to
 * swap whole packet buffers on a table-basis rather than of individual
 * descriptors.
 *
 * A route is configured by linking the configuration of one SpaceWire core to
 * another and activating interrupt controlled mode. Incoming packets are then
 * passed into the outgoing queue of the "routing" link. Routes are always
 * defined unidirectionally.
 *
 * The number of TX descriptors in a basic setup effectively limits the use of
 * the RX descriptors to be the same, otherwise stalls may occur when the number
 * of RX packets exceed the supply of TX descriptors.
 *
 *
 * ## Error Handling
 *
 * Configuration errors are indicated during setup. If an operational error
 * (e.g. DMA, cable disconnect, ...) occurs, it is reported and cleared. The
 * core is not reset or reconfigured, persistent faults will therefore raise
 * errors periodically.
 *
 *
 * ## Notes
 *
 * - there is currently support for only one DMA channel, as the core is not
 *   implemented with more channels on the GR712RC
 * - if more 64 TX or 128 RX packet slots are needed, descriptor table switching
 *   must be implemented
 * - routing is implemented inefficiently, since incoming packets are always
 *   copied to the output. The conceivably much faster zero-copy method of
 *   exchanging descriptors is not possible due to the differing descriptor
 *   layouts. Another solution would be the exchange of data buffer references,
 *   but this requires one of either:
 *	- the use of the MMU with a functioning malloc/free system
 *	- a pointer tracking system within the driver
 *	.
 *   In CHEOPS, the routing functionality is needed only for testing and
 *   calibration purposes on ground, so the copy implementation is sufficient.
 *
 *
 *
 * @example demo_spw.c
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <asm-generic/io.h>
#include <compiler.h>
#include <grspw2.h>

#include <kernel/irq.h>
#include <kernel/sysctl.h>

#if 0
#include <stdio.h>
#endif

#include <kernel/time.h>
#include <kernel/string.h>
#include <kernel/printk.h>


#if (__sparc__)
#define UINT32_T_FORMAT		"%lu"
#else
#define UINT32_T_FORMAT		"%u"
#endif

#if 0 /* XXX */
__extension__
static ssize_t rxtx_show(__attribute__((unused)) struct sysobj *sobj,
			 __attribute__((unused)) struct sobj_attribute *sattr,
			 char *buf)
{
	struct grspw2_core_cfg *cfg;


	cfg = container_of(sobj, struct grspw2_core_cfg,  sobj);

	if (!strcmp(sattr->name, "rx_bytes"))
		return sprintf(buf, UINT32_T_FORMAT, cfg->rx_bytes);

	if (!strcmp(sattr->name, "tx_bytes"))
		return sprintf(buf, UINT32_T_FORMAT, cfg->tx_bytes);

	return 0;
}

__extension__
static ssize_t rxtx_store(__attribute__((unused)) struct sysobj *sobj,
			  __attribute__((unused)) struct sobj_attribute *sattr,
			  __attribute__((unused)) const char *buf,
			  __attribute__((unused)) size_t len)
{
	struct grspw2_core_cfg *cfg;


	cfg = container_of(sobj, struct grspw2_core_cfg,  sobj);

	if (!strcmp(sattr->name, "rx_bytes")) {
		cfg->rx_bytes = 0;
		return 0;
	}

	if (!strcmp(sattr->name, "tx_bytes")) {
		cfg->tx_bytes = 0;
		return 0;
	}

	return 0;
}

__extension__
static struct sobj_attribute rx_bytes_attr = __ATTR(rx_bytes,
						    rxtx_show,
						    rxtx_store);
__extension__
static struct sobj_attribute tx_bytes_attr = __ATTR(tx_bytes,
						    rxtx_show,
						    rxtx_store);

__extension__
static struct sobj_attribute *grspw2_attributes[] = {&rx_bytes_attr,
						    &tx_bytes_attr,
						    NULL};
#endif

/**
 * @brief central error handling
 */
#define MEDIUM 1
#define LOW 1

static void grspw2_handle_error(int s)
{
#if 0
	/* XXX kalarm() */
	event_report(GRSPW2, s, (uint32_t) errno);
#endif
}


/**
 * @brief dma error interrupt callback
 * @note the error bits are cleared by *writing* with a 1, see GR712RC-UM p.128
 */

static irqreturn_t grspw2_dma_error(unsigned int irq, void *userdata)
{
	uint32_t dmactrl;
	struct grspw2_core_cfg *cfg;


	cfg = (struct grspw2_core_cfg *) userdata;

	dmactrl = ioread32be(&cfg->regs->dma[0].ctrl_status);

	if (dmactrl & GRSPW2_DMACONTROL_RA) {
#if 0		/* XXX kalarm() */
		errno = E_SPW_RX_AHB_ERROR;
#endif
		grspw2_handle_error(MEDIUM);
		dmactrl &= GRSPW2_DMACONTROL_RA;
	}

	if (dmactrl & GRSPW2_DMACONTROL_TA) {
#if 0
		errno = E_SPW_TX_AHB_ERROR;
#endif
		grspw2_handle_error(MEDIUM);
		dmactrl &= GRSPW2_DMACONTROL_TA;
	}

	iowrite32be(dmactrl, &cfg->regs->dma[0].ctrl_status);

	return 0;
}


/**
 * @brief link error interrupt callback
 */

static irqreturn_t grspw2_link_error(unsigned int irq, void *userdata)
{
	uint32_t tmp;
	uint32_t status;
	struct grspw2_core_cfg *cfg;


	cfg = (struct grspw2_core_cfg *) userdata;

	status = ioread32be(&cfg->regs->status);

	/* XXX remove tick-out stuff, this is for errors only */
	/* fast check if any bit is set, check details later */
	tmp = status & (GRSPW2_STATUS_IA | GRSPW2_STATUS_PE | GRSPW2_STATUS_DE
			| GRSPW2_STATUS_ER | GRSPW2_STATUS_CE | GRSPW2_STATUS_TO);

	if (!tmp)
		return 0;

	if (status & GRSPW2_STATUS_TO) {

		static ktime p;

		ktime c;
		static ktime drift;
		static ktime cnt;

		if (!p) {
			p = ktime_get();
			goto exit;
		}

		c = ktime_get();
		drift = c - p - cnt * 1000000000ULL - 1000000000ULL;
		cnt++;


		status &= GRSPW2_STATUS_TO;

		printk("abs:: %lld ns;\n", drift);
	}
exit:
	
	if (status & GRSPW2_STATUS_IA) {
#if 0		/* XXX kalarm() */
		errno = E_SPW_INVALID_ADDR_ERROR;
#endif
		grspw2_handle_error(MEDIUM);
		status &= GRSPW2_STATUS_IA;
	}

	if (status & GRSPW2_STATUS_PE) {
#if 0		/* XXX kalarm() */
		errno = E_SPW_PARITY_ERROR;
#endif
		grspw2_handle_error(MEDIUM);
		status &= GRSPW2_STATUS_PE;
	}

	if (status & GRSPW2_STATUS_DE) {
#if 0		/* XXX kalarm() */
		errno = E_SPW_DISCONNECT_ERROR;
#endif
		grspw2_handle_error(MEDIUM);
		status &= GRSPW2_STATUS_DE;
	}

	if (status & GRSPW2_STATUS_ER) {
#if 0		/* XXX kalarm() */
		errno = E_SPW_ESCAPE_ERROR;
#endif
		grspw2_handle_error(MEDIUM);
		status &= GRSPW2_STATUS_ER;
	}

	if (status & GRSPW2_STATUS_CE) {
#if 0		/* XXX kalarm() */
		errno = E_SPW_CREDIT_ERROR;
#endif
		grspw2_handle_error(MEDIUM);
		status &= GRSPW2_STATUS_CE;
	}

	iowrite32be(status, &cfg->regs->status);

	return 0;
}


/**
 *
 * @brief stop DMA channel operation
 *	- preserve channel settings (bits 12-16)
 *	- set abort TX bit (9)
 *	- clear status and descriptor bits (all others)
 */

static void grspw2_dma_stop(struct grspw2_regs *regs, uint32_t channel)
{
	uint32_t dmactrl;


	dmactrl  = ioread32be(&regs->dma[channel].ctrl_status);
	dmactrl &= (GRSPW2_DMACONTROL_LE | GRSPW2_DMACONTROL_SP
		    | GRSPW2_DMACONTROL_SA | GRSPW2_DMACONTROL_EN
		    | GRSPW2_DMACONTROL_NS);
	dmactrl |= GRSPW2_DMACONTROL_AT;

	iowrite32be(dmactrl, &regs->dma[channel].ctrl_status);
}


/**
 * @brief reset a DMA channel
 */

static void grspw2_dma_reset(struct grspw2_regs *regs, uint32_t channel)
{
	uint32_t dmactrl;


	dmactrl  = ioread32be(&regs->dma[channel].ctrl_status);
	dmactrl &= (GRSPW2_DMACONTROL_LE | GRSPW2_DMACONTROL_EN);

	iowrite32be(dmactrl,		&regs->dma[channel].ctrl_status);

	iowrite32be(GRSPW2_DEFAULT_MTU,	&regs->dma[channel].rx_max_pkt_len);
	iowrite32be(0x0,		&regs->dma[channel].tx_desc_table_addr);
	iowrite32be(0x0,		&regs->dma[channel].rx_desc_table_addr);
}


/**
 * @brief enable AHB error interrupt generation
 */
static void grspw2_set_ahb_irq(struct grspw2_core_cfg *cfg)
{
	uint32_t dmactrl;

	dmactrl  = ioread32be(&cfg->regs->dma[0].ctrl_status);
	dmactrl |= GRSPW2_DMACONTROL_AI;
	iowrite32be(dmactrl, &cfg->regs->dma[0].ctrl_status);
}

/**
 * @brief disable AHB error interrupt generation
 */

__attribute__((unused))
static void grspw2_unset_ahb_irq(struct grspw2_core_cfg *cfg)
{
	uint32_t dmactrl;

	dmactrl  = ioread32be(&cfg->regs->dma[0].ctrl_status);
	dmactrl &= ~GRSPW2_DMACONTROL_AI;
}

/**
 * @brief soft reset a SpW core
 *	- do NOT touch control register!
 *	- clear status bits
 *	- clear timer register
 */

static void grspw2_spw_softreset(struct grspw2_regs *regs)
{
	uint32_t ctrl, status, i;


	ctrl = ioread32be(&regs->ctrl);

	for (i = 0; i < GRSPW2_CTRL_GET_NCH(ctrl); i++)
		grspw2_dma_reset(regs, i);

	status  = ioread32be(&regs->status);
	status |= (GRSPW2_STATUS_TO | GRSPW2_STATUS_CE | GRSPW2_STATUS_ER
		   | GRSPW2_STATUS_DE | GRSPW2_STATUS_PE | GRSPW2_STATUS_IA
		   | GRSPW2_STATUS_EE);

	iowrite32be(status, &regs->status);
	iowrite32be(0x0,    &regs->time);
}


/**
 * @brief hard reset a SpW core
 *	- set reset bit high
 *	- clear timer register
 */

void grspw2_spw_hardreset(struct grspw2_regs *regs)
{
	uint32_t ctrl;


	ctrl  = ioread32be(&regs->ctrl);
	ctrl |= GRSPW2_CTRL_RS;

	iowrite32be(ctrl, &regs->ctrl);
	iowrite32be(0x0,  &regs->time);
}

/**
 * stop operation on a particular SpW link
 *	- stop all of the links DMA channels
 *	- keep link enabled (and do not touch the reset bit)
 *	- disable interrrupts
 *	- disable promiscuous mode
 *	- disable timecode reception/transmission
 *	- keep port force/select setting
 *	=> RMAP operation is not stopped on RMAP-capable links
 */

static void grspw2_spw_stop(struct grspw2_regs *regs)
{
	uint32_t ctrl, i;


	ctrl = ioread32be(&regs->ctrl);

	for (i = 0; i < GRSPW2_CTRL_GET_NCH(ctrl); i++)
		grspw2_dma_stop(regs, i);

	ctrl &= (GRSPW2_CTRL_LD | GRSPW2_CTRL_LS | GRSPW2_CTRL_NP
		 | GRSPW2_CTRL_AS | GRSPW2_CTRL_RE | GRSPW2_CTRL_RD
		 | GRSPW2_CTRL_PS | GRSPW2_CTRL_NP);

	iowrite32be(ctrl, &regs->ctrl);
}

/**
 * @brief set the address of a SpW node
 */

static void grspw2_set_node_addr(struct grspw2_regs *regs, uint8_t nodeaddr)
{
	iowrite32be((nodeaddr & GRSPW2_DMA_CHANNEL_ADDR_REG_MASK),
		    &regs->nodeaddr);
}


/**
 * @brief set the destination key of a SpW node
 * XXX: should be static and part of core configuration
 */

void grspw2_set_dest_key(struct grspw2_regs *regs, uint8_t destkey)
{
	iowrite32be((uint32_t) destkey, &regs->destkey);
}


/**
 * @brief enable rmap command processing
 */

void grspw2_set_rmap(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl |= GRSPW2_CTRL_RE;

	iowrite32be(ctrl, &cfg->regs->ctrl);
}


/**
 * @brief disable rmap command processing
 */

void grspw2_clear_rmap(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl &= ~GRSPW2_CTRL_RE;

	iowrite32be(ctrl, &cfg->regs->ctrl);
}



/**
 * @brief enable promiscuous mode
 */

void grspw2_set_promiscuous(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl |= GRSPW2_CTRL_PM;

	iowrite32be(ctrl, &cfg->regs->ctrl);
}


/**
 * @brief disable promiscuous mode
 */

void grspw2_unset_promiscuous(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl &= ~GRSPW2_CTRL_PM;

	iowrite32be(ctrl, &cfg->regs->ctrl);
}


/**
 * @brief enable autostart
 */

static void grspw2_set_autostart(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl |= GRSPW2_CTRL_AS;
	iowrite32be(ctrl, &cfg->regs->ctrl);
}


/**
 * @brief disable autostart
 */

__attribute__((unused))
static void grspw2_unset_autostart(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl &= ~GRSPW2_CTRL_AS;
	iowrite32be(ctrl, &cfg->regs->ctrl);
}


/**
 * @brief enable linkstart
 */

static void grspw2_set_linkstart(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl |= GRSPW2_CTRL_LS;
	iowrite32be(ctrl, &cfg->regs->ctrl);
}


/**
 * @brief disable linkstart
 */

__attribute__((unused))
static void grspw2_unset_linkstart(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl &= ~GRSPW2_CTRL_LS;
	iowrite32be(ctrl, &cfg->regs->ctrl);
}


/**
 * @brief enable link error interrupts
 */

void grspw2_set_link_error_irq(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl |= GRSPW2_CTRL_LI | GRSPW2_CTRL_IE;
	iowrite32be(ctrl, &cfg->regs->ctrl);
}


/**
 * @brief disable link error interrupts
 */

void grspw2_unset_link_error_irq(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl &= ~(GRSPW2_CTRL_LI | GRSPW2_CTRL_IE);
	iowrite32be(ctrl, &cfg->regs->ctrl);
}


/**
 * @brief enable time code reception
 */

void grspw2_set_time_rx(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl |= GRSPW2_CTRL_TR;
	iowrite32be(ctrl, &cfg->regs->ctrl);
}


/**
 * @brief enable time code transmission
 */

static void grspw2_set_time_tx(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl |= GRSPW2_CTRL_TT;
	iowrite32be(ctrl, &cfg->regs->ctrl);
}


/**
 * @brief disable time code transmission
 */

__attribute__((unused))
static void grspw2_unset_time_tx(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl &= ~GRSPW2_CTRL_TT;
	iowrite32be(ctrl, &cfg->regs->ctrl);
}



/**
 * @brief clear status register bits
 */

__attribute__((unused))
static void grspw2_clear_status(struct grspw2_core_cfg *cfg)
{
	uint32_t status;

	status  = ioread32be(&cfg->regs->status);
	status |= GRSPW2_STATUS_CLEAR_MASK;
	iowrite32be(status, &cfg->regs->status);
}

/**
 * @brief set Receive Interrupt enable bit in the DMA register
 */

__attribute__((unused))
static void grspw2_rx_interrupt_enable(struct grspw2_core_cfg *cfg)
{
	uint32_t flags;

	flags  = ioread32be(&cfg->regs->dma[0].ctrl_status);
	flags |= GRSPW2_DMACONTROL_RI;

	iowrite32be(flags, &cfg->regs->dma[0].ctrl_status);
}

/**
 * @brief clear Receive Interrupt enable bit in the DMA register
 */

__attribute__((unused))
static void grspw2_rx_interrupt_disable(struct grspw2_core_cfg *cfg)
{
	uint32_t flags;

	flags  = ioread32be(&cfg->regs->dma[0].ctrl_status);
	flags &= ~GRSPW2_DMACONTROL_RI;

	iowrite32be(flags, &cfg->regs->dma[0].ctrl_status);
}


/**
 * @brief set Transmit Interrupt enable bit in the DMA register
 */

__attribute__((unused))
static void grspw2_tx_interrupt_enable(struct grspw2_core_cfg *cfg)
{
	uint32_t flags;

	flags  = ioread32be(&cfg->regs->dma[0].ctrl_status);
	flags |= GRSPW2_DMACONTROL_TI;

	iowrite32be(flags, &cfg->regs->dma[0].ctrl_status);
}



/**
 * @brief clear Transmit Interrupt enable bit in the DMA register
 */

__attribute__((unused))
static void grspw2_tx_interrupt_disable(struct grspw2_core_cfg *cfg)
{
	uint32_t flags;

	flags  = ioread32be(&cfg->regs->dma[0].ctrl_status);
	flags &= ~GRSPW2_DMACONTROL_TI;

	iowrite32be(flags, &cfg->regs->dma[0].ctrl_status);
}


/**
 * @brief enable tick out interrupt
 */

void grspw2_tick_out_interrupt_enable(struct grspw2_core_cfg *cfg)
{
	uint32_t flags;

	flags  = ioread32be(&cfg->regs->ctrl);
	flags |= GRSPW2_CTRL_TQ;

	iowrite32be(flags, &cfg->regs->ctrl);

}


/**
 * @brief set SpW clock divisor
 *
 * Note: divides the external clock
 * Note: inputs are ACTUAL divisor values
 */

static int32_t grspw2_set_clockdivs(struct grspw2_regs *regs,
				    uint8_t link_start,
				    uint8_t link_run)
{
	uint32_t clkdiv;

	if (link_start && link_run) {
		/** (actual divisor value = reg value + 1) */
		clkdiv  = ((link_start - 1) << GRSPW2_CLOCKDIV_START_BIT)
			& GRSPW2_CLOCKDIV_START_MASK;
		clkdiv |= (link_run - 1) & GRSPW2_CLOCKDIV_RUN_MASK;

		iowrite32be(clkdiv, &regs->clkdiv);

		return 0;

	} else {
#if 0		/* XXX kalarm() */
		errno = E_SPW_CLOCKS_INVALID;
#endif
		return -1;
	}
}


/**
 * @brief set the rx descriptor table address; can be used to
 *        switch between multiple descriptor tables
 */

static int32_t grspw2_set_rx_desc_table_addr(struct grspw2_core_cfg *cfg,
					     uint32_t *mem)
{
	if (!mem) {
#if 0		/* XXX kalarm() */
		errno = EINVAL;
#endif
		return -1;
	}

	if (((uint32_t) mem) & GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN) {
#if 0		/* XXX kalarm() */
		errno = E_SPW_RX_DESC_TABLE_ALIGN;
#endif
		return -1;
	}

	iowrite32be(((uint32_t) mem), &cfg->regs->dma[0].rx_desc_table_addr);

	return 0;
}


/**
 * @brief set the tx descriptor table address; can be used to
 *        switch between multiple descriptor tables
 */

static int32_t grspw2_set_tx_desc_table_addr(struct grspw2_core_cfg *cfg,
					     uint32_t *mem)
{
	if (!mem) {
#if 0		/* XXX kalarm() */
		errno = EINVAL;
#endif
		return -1;
	}

	if (((uint32_t) mem) & GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN) {
#if 0		/* XXX kalarm() */
		errno = E_SPW_TX_DESC_TABLE_ALIGN;
#endif
		return -1;
	}

	iowrite32be(((uint32_t) mem), &cfg->regs->dma[0].tx_desc_table_addr);

	return 0;
}


/**
 * @brief init rx descriptor table pointers and attach to "free" list
 */

int32_t grspw2_rx_desc_table_init(struct grspw2_core_cfg *cfg,
				  uint32_t *mem,     uint32_t  tbl_size,
				  uint8_t  *pkt_buf, uint32_t  pkt_size)
{
	size_t i;

	uint32_t idx;
	uint32_t num_desc;


	if (!pkt_buf) {
#if 0		/* XXX kalarm() */
		errno = EINVAL;
#endif
		return -1;
	}

	if (grspw2_set_rx_desc_table_addr(cfg, mem))
		return -1; /* errno set in call */

	INIT_LIST_HEAD(&cfg->rx_desc_ring_used);
	INIT_LIST_HEAD(&cfg->rx_desc_ring_free);

	num_desc = tbl_size / GRSPW2_RX_DESC_SIZE;

	if (num_desc > GRSPW2_RX_DESCRIPTORS)
		num_desc = GRSPW2_RX_DESCRIPTORS;

	for (i = 0; i < num_desc; i++) {

		idx = i * GRSPW2_RX_DESC_SIZE / sizeof(uint32_t);
		cfg->rx_desc_ring[i].desc = (struct grspw2_rx_desc *) &mem[idx];

		idx = i * pkt_size;
		cfg->rx_desc_ring[i].desc->pkt_addr = (uint32_t) &pkt_buf[idx];

		/* control flags must be set elsewhere */
		cfg->rx_desc_ring[i].desc->pkt_ctrl = pkt_size;

		list_add_tail(&cfg->rx_desc_ring[i].node,
			      &cfg->rx_desc_ring_free);
	}

	return 0;
}


/**
 * @brief init tx descriptor table pointers and attach to "free" list
 */

int32_t grspw2_tx_desc_table_init(struct grspw2_core_cfg *cfg,
				  uint32_t *mem,     uint32_t  tbl_size,
				  uint8_t *hdr_buf,  uint32_t  hdr_size,
				  uint8_t *data_buf, uint32_t  data_size)
{
	uint32_t i;
	uint32_t idx;
	uint32_t num_desc;


	if (!data_buf) {
#if 0		/* XXX kalarm() */
		errno = EINVAL;
#endif
		return -1;
	}

	if (hdr_size && !hdr_buf) {
#if 0		/* XXX kalarm() */
		errno = EINVAL;
#endif
		return -1;
	}

	if (grspw2_set_tx_desc_table_addr(cfg, mem))
		return -1; /* errno set in call */

	INIT_LIST_HEAD(&cfg->tx_desc_ring_used);
	INIT_LIST_HEAD(&cfg->tx_desc_ring_free);

	num_desc = tbl_size / GRSPW2_TX_DESC_SIZE;

	if (num_desc > GRSPW2_TX_DESCRIPTORS)
		num_desc = GRSPW2_TX_DESCRIPTORS;

	for (i = 0; i < num_desc; i++) {

		idx = i * GRSPW2_TX_DESC_SIZE / sizeof(uint32_t);
		cfg->tx_desc_ring[i].desc = (struct grspw2_tx_desc *) &mem[idx];

		idx = i * hdr_size;
		cfg->tx_desc_ring[i].desc->hdr_addr = (uint32_t) &hdr_buf[idx];

		idx = i * data_size;
		cfg->tx_desc_ring[i].desc->data_addr = (uint32_t)
								&data_buf[idx];

		cfg->tx_desc_ring[i].desc->hdr_size  = hdr_size;
		cfg->tx_desc_ring[i].desc->data_size = data_size;

		/* clear packet control field, so random bits do not
		 * mark the descriptor as enabled
		 */
		cfg->tx_desc_ring[i].desc->pkt_ctrl  = 0;

		list_add_tail(&cfg->tx_desc_ring[i].node,
			      &cfg->tx_desc_ring_free);
	}

	return 0;
}

/**
 * @brief inform the core that new rx descriptors are available
 */


static void grspw2_rx_desc_new_avail(struct grspw2_core_cfg *cfg)
{
	uint32_t flags;

	flags  = ioread32be(&cfg->regs->dma[0].ctrl_status);
	flags |= GRSPW2_DMACONTROL_RD | GRSPW2_DMACONTROL_RE;

	iowrite32be(flags, &cfg->regs->dma[0].ctrl_status);
}


/**
 * @brief inform the core that new tx descriptors are available
 */

static void grspw2_tx_desc_new_avail(struct grspw2_core_cfg *cfg)
{
	uint32_t flags;

	flags  = ioread32be(&cfg->regs->dma[0].ctrl_status);
	flags |= GRSPW2_DMACONTROL_TE;

	iowrite32be(flags, &cfg->regs->dma[0].ctrl_status);
}


/**
 * @brief	move a rx descriptor to the free ring
 */

static void grspw2_rx_desc_move_free(struct grspw2_core_cfg *cfg,
			      struct grspw2_rx_desc_ring_elem *p_elem)
{
	list_move_tail(&p_elem->node, &cfg->rx_desc_ring_free);
}


/**
 * @brief	move a tx descriptor to the free ring
 */

static void grspw2_tx_desc_move_free(struct grspw2_core_cfg *cfg,
			      struct grspw2_tx_desc_ring_elem *p_elem)
{
	list_move_tail(&p_elem->node, &cfg->tx_desc_ring_free);
}


/**
 * @brief	move all inactive tx descriptors to the free ring
 */

__attribute__((unused))
static void grspw2_tx_desc_move_free_all(struct grspw2_core_cfg *cfg)
{
	struct grspw2_tx_desc_ring_elem *p_elem;
	struct grspw2_tx_desc_ring_elem *p_tmp;

	list_for_each_entry_safe(p_elem, p_tmp,
				 &cfg->tx_desc_ring_used, node) {

		if (p_elem->desc->pkt_ctrl & GRSPW2_TX_DESC_EN)
			break;

		grspw2_tx_desc_move_free(cfg, p_elem);
	}
}


/*
 * @brief	clear the busy ring of all rx descriptors (and move them
 *		to the free ring)
 */

__attribute__((unused))
static void grspw2_rx_desc_clear_all(struct grspw2_core_cfg *cfg)
{
	struct grspw2_rx_desc_ring_elem *p_elem;
	struct grspw2_rx_desc_ring_elem *p_tmp;

	list_for_each_entry_safe(p_elem, p_tmp,
				 &cfg->rx_desc_ring_used, node) {

		grspw2_rx_desc_move_free(cfg, p_elem);
	}
}


/**
 * @brief	move a rx descriptor to the busy ring
 */

static void grspw2_rx_desc_move_busy(struct grspw2_core_cfg *cfg,
			      struct grspw2_rx_desc_ring_elem *p_elem)
{
	list_move_tail(&p_elem->node, &cfg->rx_desc_ring_used);
}


/**
 * @brief	move a tx descriptor to the busy ring
 */

static void grspw2_tx_desc_move_busy(struct grspw2_core_cfg *cfg,
			      struct grspw2_tx_desc_ring_elem *p_elem)
{
	list_move_tail(&p_elem->node, &cfg->tx_desc_ring_used);

}



/**
 * @brief	retrieve a free rx descriptor
 */

static struct grspw2_rx_desc_ring_elem
	*grspw2_rx_desc_get_next_free(struct grspw2_core_cfg *cfg)
{
	struct grspw2_rx_desc_ring_elem *p_elem;

	if (likely(list_filled(&cfg->rx_desc_ring_free))) {
		p_elem = list_entry((&cfg->rx_desc_ring_free)->next,
				    struct grspw2_rx_desc_ring_elem, node);
		return p_elem;
	} else {
		return NULL;
	}
}

/**
 * @brief	retrieve a busy rx descriptor
 */

static struct grspw2_rx_desc_ring_elem
	*grspw2_rx_desc_get_next_used(struct grspw2_core_cfg *cfg)
{
	struct grspw2_rx_desc_ring_elem *p_elem;

	if (likely(list_filled(&cfg->rx_desc_ring_used))) {
		p_elem = list_entry((&cfg->rx_desc_ring_used)->next,
				    struct grspw2_rx_desc_ring_elem, node);
		return p_elem;
	} else {
		return NULL;
	}
}


/**
 * @brief	retrieve a free tx descriptor
 */

static struct grspw2_tx_desc_ring_elem
		*grspw2_tx_desc_get_next_free(struct grspw2_core_cfg *cfg)
{
	struct grspw2_tx_desc_ring_elem *p_elem;

	if (likely(list_filled(&cfg->tx_desc_ring_free))) {
		p_elem = list_entry((&cfg->tx_desc_ring_free)->next,
				    struct grspw2_tx_desc_ring_elem, node);
		return p_elem;
	} else {
		return NULL;
	}
}


/**
 * @brief	set tx descriptor active
 * @note	per-packet interrupt are enabled by default, enable rx irq
 *		in dma control register to have them actually fire
 */

static void grspw2_rx_desc_set_active(struct grspw2_rx_desc_ring_elem *p_elem)
{
	p_elem->desc->pkt_ctrl |= GRSPW2_RX_DESC_IE | GRSPW2_RX_DESC_EN;
}


/**
 * @brief	set tx descriptor active
 */

static void grspw2_tx_desc_set_active(struct grspw2_tx_desc_ring_elem *p_elem)
{
	p_elem->desc->pkt_ctrl |= GRSPW2_TX_DESC_IE | GRSPW2_TX_DESC_EN;
}

/**
 * @brief	check if there are rx descriptors in the free ring
 *
 * @return	1 if available, 0 if ring is empty
 */

static int32_t grspw2_rx_desc_avail(struct grspw2_core_cfg *cfg)
{
	if (likely(list_filled(&cfg->rx_desc_ring_free)))
		return 1;
	else
		return 0;
}


/**
 * @brief	check if there are tx descriptors in the free ring
 *
 * @return	1 if available, 0 if ring is empty
 */

__attribute__((unused))
static int32_t grspw2_tx_desc_avail(struct grspw2_core_cfg *cfg)
{
	if (likely(list_filled(&cfg->tx_desc_ring_free)))
		return 1;
	else
		return 0;
}


/**
 * @brief	try to activate a free descriptor
 * @return	0 on success, -1 if no descriptor is available
 *
 * @note There is no check for NULL on grspw2_rx_desc_get_next_free(), because
 *       it would be redundant, since a positive return on
 *       grspw2_rx_desc_avail() guarantees the availability of a descriptor
 *       element. (Unless someone maliciously fiddles with our lists
 *       externally). We could of course check anyway, but this function should
 *       be snappy, if we can avoid something clearly useless, we should.
 */

static int32_t grspw2_rx_desc_add(struct grspw2_core_cfg *cfg)
{
	struct grspw2_rx_desc_ring_elem *p_elem;


	if (!grspw2_rx_desc_avail(cfg))
		return -1;
	/* 
	   The case p_elem == NULL is taken care for by the above check. 
	   Also see the note in the function description.
	*/
	p_elem = grspw2_rx_desc_get_next_free(cfg);

	grspw2_rx_desc_set_active(p_elem);

	grspw2_rx_desc_move_busy(cfg, p_elem);

	grspw2_rx_desc_new_avail(cfg);

	return 0;
}


static void grswp2_rx_desc_add_all(struct grspw2_core_cfg *cfg)
{
	size_t i;

	for (i = 0; i < GRSPW2_RX_DESCRIPTORS; i++) {
		if (grspw2_rx_desc_add(cfg))
			break;
	}
}


/**
 * @brief set up a RX descriptor with a custom address and control field
 *
 * @return 0 on success, -1 if no descriptors were available
 */

__attribute__((unused))
static int32_t grspw2_rx_desc_add_custom(struct grspw2_core_cfg *cfg,
				  uint32_t pkt_ctrl,
				  uint32_t pkt_addr)
{

	struct grspw2_rx_desc_ring_elem *p_elem;

	p_elem = grspw2_rx_desc_get_next_free(cfg);

	if (unlikely(!p_elem)) {
#if 0		/* XXX kalarm() */
		errno = E_SPW_NO_RX_DESC_AVAIL;
#endif
		return -1;
	}

	/* ALWAYS set packet address first! */
	p_elem->desc->pkt_addr = pkt_addr;
	p_elem->desc->pkt_ctrl = pkt_ctrl;

	list_move_tail(&p_elem->node, &cfg->rx_desc_ring_used);

	grspw2_rx_desc_new_avail(cfg);

	return 0;
}


/**
 * @brief	release and try to reactivate a descriptor
 * @return	0 on success, -1 if no descriptor is available
 */

static int32_t grspw2_rx_desc_readd(struct grspw2_core_cfg *cfg,
			     struct grspw2_rx_desc_ring_elem *p_elem)
{
	grspw2_rx_desc_move_free(cfg, p_elem);

	return grspw2_rx_desc_add(cfg);
}

/**
 * @brief	try to activate a free descriptor and copy the data from the
 *		supplied buffer
 *
 * @return	0 on success, -1 on failure
 *
 * @note	USE WITH CARE: does not perform any size checks on the buffers
 *
 */

static int32_t grspw2_tx_desc_add_pkt(struct grspw2_core_cfg *cfg,
				      bool rmap_pkt,
				      const void *hdr_buf,
				      uint32_t hdr_size,
				      uint8_t non_crc_bytes,
				      const void *data_buf,
				      uint32_t data_size)
{
	struct grspw2_tx_desc_ring_elem *p_elem;


	/* this is a little excessive, remove on your own risk */

	if (hdr_size && !hdr_buf) {
#if 0		/* XXX kalarm() */
		errno = EINVAL;
#endif
		return -1;
	}

	if (data_size && !data_buf) {
#if 0		/* XXX kalarm() */
		errno = EINVAL;
#endif
		return -1;
	}

	grspw2_tx_desc_move_free_all(cfg);

	p_elem = grspw2_tx_desc_get_next_free(cfg);


	if (unlikely(!p_elem)) {
#if 0		/* XXX kalarm() */
		errno = E_SPW_NO_TX_DESC_AVAIL;
#endif
		return -1;
	}

	/* XXX need extra sanity checks somewhere in the interface,
	 * could be that no headers are alloced! or data size is too small! */
	if (hdr_buf != NULL) 
		memcpy((void *) p_elem->desc->hdr_addr,  hdr_buf,  hdr_size);

	if (data_buf != NULL)
		memcpy((void *) p_elem->desc->data_addr, data_buf, data_size);

	p_elem->desc->hdr_size  = hdr_size;
	p_elem->desc->data_size = data_size;

	if (rmap_pkt) {
		if (hdr_size)
			p_elem->desc->append_header_crc = 1;
		if (data_size)
			p_elem->desc->append_data_crc = 1;
		if (non_crc_bytes)
			p_elem->desc->non_crc_bytes = non_crc_bytes & 0xF;
	}

	grspw2_tx_desc_set_active(p_elem);

	grspw2_tx_desc_move_busy(cfg, p_elem);

	grspw2_tx_desc_new_avail(cfg);

	return 0;

}


/**
 * @brief set up a TX descriptor with a custom header/data and control field.
 *
 * @return 0 on success, -1 if no descriptors were available
 */
__attribute__((unused))
static int32_t grspw2_tx_desc_add_custom(struct grspw2_core_cfg *cfg,
				  uint32_t pkt_ctrl,
				  uint32_t hdr_size,
				  uint32_t hdr_addr,
				  uint32_t data_size,
				  uint32_t data_addr)
{
	struct grspw2_tx_desc_ring_elem *p_elem;


	grspw2_tx_desc_move_free_all(cfg);

	p_elem = grspw2_tx_desc_get_next_free(cfg);

	if (unlikely(!p_elem)) {
#if 0		/* XXX kalarm() */
		errno = E_SPW_NO_TX_DESC_AVAIL;
#endif
		return -1;
	}

	p_elem->desc->hdr_size = hdr_size;
	p_elem->desc->hdr_addr = hdr_addr;

	p_elem->desc->data_size = data_size;
	p_elem->desc->data_addr = data_addr;

	p_elem->desc->pkt_ctrl = pkt_ctrl;

	list_move_tail(&p_elem->node, &cfg->tx_desc_ring_used);

	grspw2_tx_desc_new_avail(cfg);

	return 0;
}

/**
 *  @brief configure maximum transmission unit
 */

static void grspw2_set_mtu(struct grspw2_core_cfg *cfg, uint32_t mtu)
{
	iowrite32be(mtu, &cfg->regs->dma[0].rx_max_pkt_len);
}


/**
 * @brief default dma configuration, see GR712RC UM, p.128 for flags
 */

static void grspw2_configure_dma(struct grspw2_core_cfg *cfg)
{
	uint32_t flags;

	flags = GRSPW2_DMACONTROL_AI | GRSPW2_DMACONTROL_PS
		| GRSPW2_DMACONTROL_PR | GRSPW2_DMACONTROL_TA
		| GRSPW2_DMACONTROL_RA | GRSPW2_DMACONTROL_NS;

	iowrite32be(flags, &cfg->regs->dma[0].ctrl_status);
}


/**
 * @brief get timecnt field of grspw2 time register
 */

uint32_t grspw2_get_timecnt(struct grspw2_core_cfg *cfg)
{
	return ioread32be(&cfg->regs->time) & GRSPW2_TIME_TIMECNT;
}



/**
 * @brief get link status
 */

uint32_t grspw2_get_link_status(struct grspw2_core_cfg *cfg)
{
	return GRSPW2_STATUS_GET_LS(ioread32be(&cfg->regs->status));
}



/**
 * @brief transmit a SpW time code; there should be at least 4 system
 *        clocks and 25 transmit clocks inbetween calls, see GR712RC UM, p.108
 */

void grspw2_tick_in(struct grspw2_core_cfg *cfg)
{
	uint32_t ctrl;

	ctrl  = ioread32be(&cfg->regs->ctrl);
	ctrl |= GRSPW2_CTRL_TI;

	iowrite32be(ctrl, &cfg->regs->ctrl);
}

#ifdef CONFIG_GRSPW2_SPW_ROUTING_
/**
 * @brief	interrupt handler for packet routing
 * @note	only a single node is supported at the moment
 */

int32_t grspw2_route(void *userdata)
{
	int32_t ret;

	struct grspw2_core_cfg *cfg;

	struct grspw2_rx_desc_ring_elem *p_elem;
	struct grspw2_rx_desc_ring_elem *p_tmp;

	cfg = (struct grspw2_core_cfg *) userdata;

	list_for_each_entry_safe(p_elem, p_tmp, &cfg->rx_desc_ring_used, node) {

		if (p_elem->desc->pkt_ctrl & GRSPW2_RX_DESC_EN)
			break;

		/* copy to output if there is a free tx descriptor */
		ret = grspw2_tx_desc_add_pkt(cfg->route[0],
					     false,
					     NULL,
					     0,
					     0,
					     (void *) p_elem->desc->pkt_addr,
					     p_elem->desc->pkt_size);

		if (unlikely(ret))
			break;

		grspw2_rx_desc_readd(cfg, p_elem);
	}

	grspw2_tx_desc_move_free_all(cfg->route[0]);

	return 0;
}


/**
 * @brief enable routing between SpW cores
 */

int32_t grspw2_enable_routing(struct grspw2_core_cfg *cfg,
			      struct grspw2_core_cfg *route)
{
	size_t i;


	cfg->route[0] = route;

	irl2_register_callback(cfg->core_irq, PRIORITY_NOW,
			       grspw2_route, (void *) cfg);


	grspw2_set_promiscuous(cfg);
	grspw2_rx_interrupt_enable(cfg);

	/* only allow as many rx descriptors as there are tx descriptors
	 * or we could run into a jam, because we do not use interrupts on TX
	 * and hence do not reload the ring if it happens
	 */
	grspw2_rx_desc_clear_all(cfg);

	for (i = 0; i < GRSPW2_TX_DESCRIPTORS; i++)
		grspw2_rx_desc_add(cfg);

	return 0;
}


/**
 * @brief enable interactive routing between SpW cores
 * @note call grspw2_route() to route received descriptors
 */

int32_t grspw2_enable_routing_noirq(struct grspw2_core_cfg *cfg,
                                   struct grspw2_core_cfg *route)
{
	cfg->route[0] = route;

	grspw2_set_promiscuous(cfg);

	return 0;
}



/**
 * @brief disable routing between SpW cores
 */

int32_t grspw2_disable_routing(struct grspw2_core_cfg *cfg)
{
	irl2_deregister_callback(cfg->core_irq, grspw2_route,
				 (void *) cfg->route[0]);

	grspw2_unset_promiscuous(cfg);
	grspw2_rx_interrupt_disable(cfg);

	return 0;
}
#endif /* (__SPW_ROUTING__) */


/**
 * @brief retrieve number of packets available
 */

uint32_t grspw2_get_num_pkts_avail(struct grspw2_core_cfg *cfg)
{
	uint32_t i = 0;

	struct grspw2_rx_desc_ring_elem *p_elem;
	struct grspw2_rx_desc_ring_elem *p_tmp;

	list_for_each_entry_safe(p_elem, p_tmp, &cfg->rx_desc_ring_used, node) {
		if (p_elem->desc->pkt_ctrl & GRSPW2_RX_DESC_EN)
			break;
		i++;
	}

	return i;
}


/**
 * @brief get number of available free tx descriptors
 */

uint32_t grspw2_get_num_free_tx_desc_avail(struct grspw2_core_cfg *cfg)
{
	uint32_t i = 0;

	struct grspw2_tx_desc_ring_elem *p_elem;
	struct grspw2_tx_desc_ring_elem *p_tmp;

	list_for_each_entry_safe(p_elem, p_tmp, &cfg->tx_desc_ring_free, node) {
		i++;
	}

	return i;
}


/**
 * @brief retrieve the size of the next packet
 */

uint32_t grspw2_get_next_pkt_size(struct grspw2_core_cfg *cfg)
{
	uint32_t pkt_size;

	struct grspw2_rx_desc_ring_elem *p_elem;


	p_elem = grspw2_rx_desc_get_next_used(cfg);

	if (!p_elem)
		return 0;

	/* still active */
	if (p_elem->desc->pkt_ctrl & GRSPW2_RX_DESC_EN)
		return 0;

	pkt_size = p_elem->desc->pkt_size - cfg->strip_hdr_bytes;

	return pkt_size;
}



/**
 * @brief retrieve a packet
 *
 * @param pkt if the packet return buffer
 *	  if NULL, the size of the pending packet is returned
 */

uint32_t grspw2_get_pkt(struct grspw2_core_cfg *cfg, uint8_t *pkt)
{
	uint32_t pkt_size;

	struct grspw2_rx_desc_ring_elem *p_elem;


	p_elem = grspw2_rx_desc_get_next_used(cfg);

	if (!p_elem)
		return 0;

	/* still active */
	if (p_elem->desc->pkt_ctrl & GRSPW2_RX_DESC_EN)
		return 0;

	pkt_size = p_elem->desc->pkt_size - cfg->strip_hdr_bytes;

	if (!pkt)
		return pkt_size;

	cfg->rx_bytes += p_elem->desc->pkt_size;

	memcpy((void *) pkt,
	       (void *) (p_elem->desc->pkt_addr + cfg->strip_hdr_bytes),
	       pkt_size);

	grspw2_rx_desc_readd(cfg, p_elem);

	return pkt_size;
}

/**
 * @brief drop a packet
 * @return 1 if packet was dropped, 0 otherwise
 */

uint32_t grspw2_drop_pkt(struct grspw2_core_cfg *cfg)
{
	struct grspw2_rx_desc_ring_elem *p_elem;


	p_elem = grspw2_rx_desc_get_next_used(cfg);

	if (!p_elem)
		return 0;

	/* still active */
	if (p_elem->desc->pkt_ctrl & GRSPW2_RX_DESC_EN)
		return 0;

	cfg->rx_bytes += p_elem->desc->pkt_size;

	grspw2_rx_desc_readd(cfg, p_elem);

	return 1;
}



/**
 * @brief add a packet
 */

int32_t grspw2_add_pkt(struct grspw2_core_cfg *cfg,
			const void *hdr,  uint32_t hdr_size,
			const void *data, uint32_t data_size)
{
	int32_t ret;


	ret = grspw2_tx_desc_add_pkt(cfg, false, hdr, hdr_size, 0,
				     data, data_size);

	if (unlikely(ret)) {
		grspw2_handle_error(LOW);
		return -1;
	}

	cfg->tx_bytes += hdr_size + data_size;

	return 0;
}


/**
 * @brief add an RMAP packet
 */

int32_t grspw2_add_rmap(struct grspw2_core_cfg *cfg,
		        const void *hdr,  uint32_t hdr_size,
		        const uint8_t non_crc_bytes,
		        const void *data, uint32_t data_size)
{
	int32_t ret;


	ret = grspw2_tx_desc_add_pkt(cfg, true, hdr, hdr_size, non_crc_bytes,
				     data, data_size);

	if (unlikely(ret)) {
		grspw2_handle_error(LOW);
		return -1;
	}

	cfg->tx_bytes += hdr_size + data_size;

	return 0;
}




/**
  * @brief start core operation
 */

void grspw2_core_start(struct grspw2_core_cfg *cfg)
{
	grswp2_rx_desc_add_all(cfg);
	grspw2_clear_status(cfg);
	grspw2_set_linkstart(cfg);
	grspw2_set_autostart(cfg);
}


/**
 * @brief (re)initialise a grswp2 core
 */
#define SYSCTL_STRING_SIZE 8

#if (__sparc__)
#define SYSCTL_NAME_FORMAT	"spw%lu"
#else
#define SYSCTL_NAME_FORMAT	"spw%u"
#endif


int32_t grspw2_core_init(struct grspw2_core_cfg *cfg, uint32_t core_addr,
			 uint8_t node_addr, uint8_t link_start,
			 uint8_t link_run, uint32_t mtu,
			 uint32_t core_irq, uint32_t ahb_irq,
			 uint32_t strip_hdr_bytes)
{
	int32_t ret;
#if 0
	char *buf;
#endif

	cfg->regs = (struct grspw2_regs *) core_addr;
	cfg->core_irq = core_irq;
	cfg->ahb_irq  = ahb_irq;

	cfg->strip_hdr_bytes = strip_hdr_bytes;

	grspw2_spw_stop(cfg->regs);
	grspw2_spw_softreset(cfg->regs);
	grspw2_set_node_addr(cfg->regs, node_addr);

#if 0
	irq_free(cfg->ahb_irq,  grspw2_dma_error,  cfg);
	irq_free(cfg->core_irq, grspw2_link_error, cfg);
#endif

	ret = grspw2_set_clockdivs(cfg->regs, link_start, link_run);
	if (ret)
		return -1;

	grspw2_set_mtu(cfg, mtu);
	grspw2_configure_dma(cfg);
	grspw2_set_time_tx(cfg);

	grspw2_set_ahb_irq(cfg);
	grspw2_set_link_error_irq(cfg);

	irq_request(cfg->ahb_irq, ISR_PRIORITY_NOW, grspw2_dma_error, cfg);

	irq_request(cfg->core_irq, ISR_PRIORITY_NOW, grspw2_link_error, cfg);

	cfg->rx_bytes = 0;
	cfg->tx_bytes = 0;

#if 0	/* XXX sysctl */
	/* as sysctl does not provide a _remove() function, make
	 * sure that we do not re-add the same object to the sysctl tree
	 */
	if (cfg->sobj.sattr)
		return 0;

	sysobj_init(&cfg->sobj);

	cfg->sobj.sattr = grspw2_attributes;

	/* derive the spw link number from the interrupt number */
	buf = (char *) malloc(SYSCTL_STRING_SIZE * sizeof(char));

	snprintf(buf, SYSCTL_STRING_SIZE * sizeof(char), SYSCTL_NAME_FORMAT,
		 core_irq - GR712_IRL2_GRSPW2_0);


	sysobj_add(&cfg->sobj, NULL, driver_set, buf);
#endif

	return 0;

	/* NOTE: here CLANG scan-view reports a false positive
	 *  about buf being a memory leak
	 */
}


#if (__sparc__)
/**
 * set the external clock to the grspw2 core
 * does not actually belong here...
 */

void set_gr712_spw_clock(void)
{
	uint32_t *gpreg = (uint32_t *) 0x80000600;

	/** set input clock to 100 MHz INCLK */
	(*gpreg) = (ioread32be(gpreg) & (0xFFFFFFF8));
}
#endif
