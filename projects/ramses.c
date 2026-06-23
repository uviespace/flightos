
#include <kernel/init.h>
#include <kernel/kmem.h>
#include <kernel/kthread.h>
#include <kernel/module.h>
#include <kernel/application.h>
#include <kernel/edac.h>
#include <kernel/memscrub.h>
#include <kernel/user.h>
#include <kernel/irq.h>

#include <grspw2.h>
#include <asm-generic/io.h>
#include <asm/leon_reg.h>
#include <modules-image.h>

#include <adc128s102.h>

#include <string.h>

#define MSG "RAMSES: "


/* a spacewire core configuration (0 = obc,  1 = red,  2 = camera */
struct spw_user_cfg spw_cfg[3];
#if 1
#define SPW_CLCKDIV_START	6
#define SPW_CLCKDIV_PLM_RUN	3		/* baseline is 20 Mbit (60 MHz inclk) */
#else
#define SPW_CLCKDIV_START      10
#define SPW_CLCKDIV_PLM_RUN	5
#endif
#define SPW_CLCKDIV_CAM_RUN	1
#define GR712_IRL1_AHBSTAT	1

#define HDR_SIZE		0x4
#define STRIP_HDR_BYTES		0x4

#define RAMSES_MTU_TM		2048
#define RAMSES_MTU_TC		GRSPW2_DEFAULT_MTU

#define RAMSES_SC_RX_NDESC	GRSPW2_RX_DESCRIPTORS
#define RAMSES_SC_TX_NDESC	GRSPW2_TX_DESCRIPTORS

#define RAMSES_CAM_TX_NDESC		3
#define RAMSES_CAM_RX_NDESC		3

#define RAMSES_DPU_ADDR_TO_OBC	0xFE
#define RAMSES_DPU_ADDR_TO_CAM  0x50

#define CAM_IMG_BUFFERS		RAMSES_CAM_RX_NDESC
#define GRSPW2_CAM_RX_MTU	(2 * 1024 * 1024)

/* this is unmanaged reserved physical memory */
#define SPW_AREA_START		0x6F800000
#define SPW_AREA_END		0x6FF00000


#define CLKGATE_GRETH		0x00000001
#define CLKGATE_GRSPW0		0x00000002
#define CLKGATE_GRSPW1		0x00000004
#define CLKGATE_GRSPW2		0x00000008
#define CLKGATE_GRSPW3		0x00000010
#define CLKGATE_GRSPW4		0x00000020
#define CLKGATE_GRSPW5		0x00000040
#define CLKGATE_CAN		0x00000080

#define CLKGATE_BASE		0x80000D00

__attribute__((unused))
static struct gr712_clkgate {
	uint32_t unlock;
	uint32_t clk_enable;
	uint32_t core_reset;
} *clkgate = (struct gr712_clkgate *)CLKGATE_BASE;


static void gr712_clkgate_enable(uint32_t gate)
{
	uint32_t flags;
	flags  = ioread32be(&clkgate->unlock);
	flags |= gate;
	iowrite32be(flags, &clkgate->unlock);

	flags  = ioread32be(&clkgate->core_reset);
	flags |= gate;
	iowrite32be(flags, &clkgate->core_reset);

	flags  = ioread32be(&clkgate->clk_enable);
	flags |= gate;
	iowrite32be(flags, &clkgate->clk_enable);

	flags  = ioread32be(&clkgate->core_reset);
	flags &= ~gate;
	iowrite32be(flags, &clkgate->core_reset);

	flags  = ioread32be(&clkgate->unlock);
	flags &= ~gate;
	iowrite32be(flags, &clkgate->unlock);
}


static void ramses_set_gr712_spw_clock(void)
{
	uint32_t *gpreg = (uint32_t *)0x80000600;



        (*gpreg) = (ioread32be(gpreg) & (0xFFFFFFF8));
#if 0
	/* set 2x spw dll so we get to 100 MHz from the 50 MHz
	 * base clock; this requires the DLL to be pulled out of
	 * reset, since it is active low!
	 */
	(*gpreg) = ((ioread32be(gpreg) & (0xFFFFFFF8))) | 0x15;
#else
	/* 60 MHz configuration (proposed flight configuration)
	 * we use the inclk for spw, no 2x dll
	 */
	(*gpreg) = ((ioread32be(gpreg) & (0xFFFFFFF8))) | 0x3;
#endif
}



/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_obc(struct spw_user_cfg *cfg, uint32_t n_rx_desc, uint32_t n_tx_desc)
{
	ramses_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW0);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_0,
			 RAMSES_DPU_ADDR_TO_OBC, SPW_CLCKDIV_START, SPW_CLCKDIV_PLM_RUN,
			 RAMSES_MTU_TC, GRSPW2_IRQ_CORE0,
			 GR712_IRL1_AHBSTAT, STRIP_HDR_BYTES);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  n_rx_desc *  GRSPW2_RX_DESC_SIZE,
				  cfg->rx_data,
				  RAMSES_MTU_TC);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  n_tx_desc * GRSPW2_TX_DESC_SIZE,
				  cfg->tx_hdr, HDR_SIZE,
				  cfg->tx_data, RAMSES_MTU_TM);

	/* XXX check which of these we need */
#if 1
	grspw2_set_promiscuous(&cfg->spw);
#endif
#if 0
	grspw2_protocol_id_drop_enable(&cfg->spw, HDR_PROTO_BYTE, HDR_PROTO_ID);
#endif
}



/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_red(struct spw_user_cfg *cfg, uint32_t n_rx_desc, uint32_t n_tx_desc)
{
	ramses_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW1);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_1,
			 RAMSES_DPU_ADDR_TO_OBC, SPW_CLCKDIV_START, SPW_CLCKDIV_PLM_RUN,
			 RAMSES_MTU_TC, GRSPW2_IRQ_CORE1,
			 GR712_IRL1_AHBSTAT, STRIP_HDR_BYTES);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  n_rx_desc *  GRSPW2_RX_DESC_SIZE,
				  cfg->rx_data,
				  RAMSES_MTU_TC);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  n_tx_desc * GRSPW2_TX_DESC_SIZE,
				  cfg->tx_hdr, HDR_SIZE,
				  cfg->tx_data, RAMSES_MTU_TM);

	/* XXX check which of these we need */
#if 1
	grspw2_set_promiscuous(&cfg->spw);
#endif
#if 0
	grspw2_protocol_id_drop_enable(&cfg->spw, HDR_PROTO_BYTE, HDR_PROTO_ID);
#endif
}


/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_camera(struct spw_user_cfg *cfg, uint32_t n_rx_desc, uint32_t n_tx_desc)
{
	ramses_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW2);

	/* configure for spw core1 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_2,
			 RAMSES_DPU_ADDR_TO_CAM, SPW_CLCKDIV_START, SPW_CLCKDIV_CAM_RUN,
			 GRSPW2_CAM_RX_MTU, GRSPW2_IRQ_CORE2,
			 GR712_IRL1_AHBSTAT, 0);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  n_rx_desc * GRSPW2_RX_DESC_SIZE,
				  cfg->rx_data,
				  GRSPW2_CAM_RX_MTU);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  n_tx_desc * GRSPW2_TX_DESC_SIZE,
				  cfg->tx_hdr, HDR_SIZE,
				  cfg->tx_data, GRSPW2_DEFAULT_MTU);

	grspw2_set_promiscuous(&cfg->spw);
}


/* emit signals 101 and 102 to indicate packet on nom/red S/C link */
static irqreturn_t emit_irq_nom(unsigned int irq, void *userdata)
{
	ksignal_send_info(101, NULL);
	return 0;
}

static irqreturn_t emit_irq_red(unsigned int irq, void *userdata)
{
	ksignal_send_info(102, NULL);
	return 0;
}

static irqreturn_t emit_irq_cam(unsigned int irq, void *userdata)
{
	ksignal_send_info(103, NULL);
	return 0;
}

static void gr712_adc128s102_spi_cs(bool enable)
{
	uint32_t dir;
	uint32_t out;

	struct leon3_grgpio_registermap *reg = (void *)LEON3_BASE_ADDRESS_GRGPIO_2;


	dir = ioread32be(&reg->ioport_direction);
	out = ioread32be(&reg->ioport_output_value);

	/* CS on ADC128S102 is active low */
	if (enable) {
		iowrite32be(dir |  (1 << 22), &reg->ioport_direction);
		iowrite32be(out & ~(1 << 22), &reg->ioport_output_value);
	} else {
		iowrite32be(out |  (1 << 22), &reg->ioport_output_value);
		iowrite32be(dir & ~(1 << 22), &reg->ioport_direction);
	}
}


static int ramses_init(void)
{
	uint8_t *addr;


	adc128s102_register(gr712_adc128s102_spi_cs);

	memset((void *)SPW_AREA_START, 0, SPW_AREA_END - SPW_AREA_START);

	/* we require 1kiB tables which are also aligned to 1kiB */
	spw_cfg[0].rx_desc = (uint32_t *)(SPW_AREA_START + 1024 * 0);
	spw_cfg[0].tx_desc = (uint32_t *)(SPW_AREA_START + 1024 * 1);
	spw_cfg[1].rx_desc = (uint32_t *)(SPW_AREA_START + 1024 * 2);
	spw_cfg[1].tx_desc = (uint32_t *)(SPW_AREA_START + 1024 * 3);

	spw_cfg[2].rx_desc = (uint32_t *)(SPW_AREA_START + 1024 * 4);
	spw_cfg[2].tx_desc = (uint32_t *)(SPW_AREA_START + 1024 * 5);

	/* note: the addresses for the header and data buffers may be byte aligned */
	addr = (uint8_t *)(SPW_AREA_START + 1024 * 6);

	/* nominal S/C link */
	spw_cfg[0].rx_data = addr;
	addr += RAMSES_MTU_TC * RAMSES_SC_RX_NDESC;
	spw_cfg[0].tx_data = addr;
	addr += RAMSES_MTU_TM * RAMSES_SC_TX_NDESC;
	spw_cfg[0].tx_hdr = addr;
	addr += HDR_SIZE * RAMSES_SC_TX_NDESC;

	/* redundant S/C link */
	spw_cfg[1].rx_data = addr;
	addr += RAMSES_MTU_TC * RAMSES_SC_RX_NDESC;
	spw_cfg[1].tx_data = addr;
	addr += RAMSES_MTU_TM * RAMSES_SC_TX_NDESC;
	spw_cfg[1].tx_hdr = addr;
	addr += HDR_SIZE * RAMSES_SC_TX_NDESC;

	/* CAM link */
	spw_cfg[2].rx_data = addr;
	addr += GRSPW2_CAM_RX_MTU * RAMSES_CAM_RX_NDESC;
	spw_cfg[2].tx_data = addr;
	addr += GRSPW2_DEFAULT_MTU * RAMSES_CAM_TX_NDESC;
	spw_cfg[2].tx_hdr = addr;
	addr += HDR_SIZE * RAMSES_CAM_TX_NDESC;


	/* final sanity check */
	BUG_ON((uintptr_t)addr >= SPW_AREA_END);


	spw_init_core_obc(&spw_cfg[0], RAMSES_SC_RX_NDESC, RAMSES_SC_TX_NDESC);

	grspw2_core_start(&spw_cfg[0].spw, 1, 1);
	grspw2_set_time_rx(&spw_cfg[0].spw);
	grspw2_tick_out_interrupt_enable(&spw_cfg[0].spw);
	grspw2_rx_interrupt_enable(&spw_cfg[0].spw);
	irq_request(spw_cfg[0].spw.core_irq, ISR_PRIORITY_NOW, emit_irq_nom, NULL);
	irq_request(GR712_IRL1_AHBSTAT, ISR_PRIORITY_NOW, emit_irq_nom, NULL);

	spw_init_core_red(&spw_cfg[1], RAMSES_SC_RX_NDESC, RAMSES_SC_TX_NDESC);

	grspw2_core_start(&spw_cfg[1].spw, 1, 1);
	grspw2_set_time_rx(&spw_cfg[1].spw);
	grspw2_tick_out_interrupt_enable(&spw_cfg[1].spw);
	grspw2_rx_interrupt_enable(&spw_cfg[1].spw);
	irq_request(spw_cfg[1].spw.core_irq, ISR_PRIORITY_NOW, emit_irq_red, NULL);
	irq_request(GR712_IRL1_AHBSTAT, ISR_PRIORITY_NOW, emit_irq_red, NULL);


	spw_init_core_camera(&spw_cfg[2], RAMSES_CAM_RX_NDESC, RAMSES_CAM_TX_NDESC);
	grspw2_core_start(&spw_cfg[2].spw, 1, 1);
	irq_request(spw_cfg[2].spw.core_irq, ISR_PRIORITY_NOW, emit_irq_cam, NULL);
	irq_request(GR712_IRL1_AHBSTAT, ISR_PRIORITY_NOW, emit_irq_cam, NULL);


#ifdef CONFIG_EMBED_APPLICATION
	/* load RAMSES ASW */
	addr = module_read_embedded("dpm");
	printk(MSG "test executable address is %p\n", addr);
	if (addr)
		application_load(addr, "ASW", 1, 0, NULL);
#endif /* CONFIG_EMBED_APPLICATION */


	return 0;
}
lvl1_usercall(ramses_init)
