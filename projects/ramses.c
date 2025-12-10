
#include <kernel/init.h>
#include <kernel/kmem.h>
#include <kernel/kthread.h>
#include <kernel/module.h>
#include <kernel/application.h>
#include <kernel/edac.h>
#include <kernel/memscrub.h>
#include <kernel/user.h>

#include <grspw2.h>
#include <asm-generic/io.h>
#include <modules-image.h>

#include <string.h>

#define MSG "RAMSES: "


/* a spacewire core configuration (0 = obc,  1 = camera */
struct spw_user_cfg spw_cfg[2];

#define SPW_CLCKDIV_START	10
#define SPW_CLCKDIV_PLM_RUN	5		/* baseline is 20 Mbit */
#define SPW_CLCKDIV_CAM_RUN	1
#define GR712_IRL1_AHBSTAT	1

#define HDR_SIZE		0x4
#define STRIP_HDR_BYTES		0x4

#define RAMSES_MTU_TM		4096		/* XXX check, this may be just 2 kiB */
#define RAMSES_MTU_TC		GRSPW2_DEFAULT_MTU

#define CAM_TX_NDESC		16
#define CAM_RX_NDESC		104

#define RAMSES_DPU_ADDR_TO_OBC	0xFE
#define RAMSES_DPU_ADDR_TO_CAM  0x50


#define CAM_IMG_BUFFERS		104
#define GRSPW2_CAM_RX_MTU	(2 * 1024*1024)

/* the start of our image buffers */
#define CAM_SPW_BUF_START	0x63000000
/* we take 4 kiB for the SpW descs from the 1 MiB reserved for the ASW */
#define SPW_DESC_START		(CAM_SPW_BUF_START - 4 * 1024)


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


	/* set 2x spw dll so we get to 100 MHz from the 50 MHz
	 * base clock; this requires the DLL to be pulled out of
	 * reset, since it is active low!
	 */
	(*gpreg) = ((ioread32be(gpreg) & (0xFFFFFFF8))) | 0x15;
}


static void spw_alloc_obc(struct spw_user_cfg *cfg)
{
	/* we have our descriptors already assigned, so
	 * malloc rx and tx data buffers: decriptors * packet size
	 */
	cfg->rx_data = (uint8_t *) kpcalloc(1, GRSPW2_RX_DESCRIPTORS * GRSPW2_DEFAULT_MTU);
	cfg->tx_data = (uint8_t *) kpcalloc(1, GRSPW2_TX_DESCRIPTORS * GRSPW2_DEFAULT_MTU);

	cfg->tx_hdr = (uint8_t *) kpcalloc(1, GRSPW2_TX_DESCRIPTORS * HDR_SIZE);
}


/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_obc(struct spw_user_cfg *cfg)
{
	ramses_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW0);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_0,
			 RAMSES_DPU_ADDR_TO_OBC, SPW_CLCKDIV_START, SPW_CLCKDIV_PLM_RUN,
			 GRSPW2_DEFAULT_MTU, GRSPW2_IRQ_CORE0,
			 GR712_IRL1_AHBSTAT, STRIP_HDR_BYTES);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  GRSPW2_DESCRIPTOR_TABLE_SIZE,
				  cfg->rx_data,
				  RAMSES_MTU_TC);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  GRSPW2_DESCRIPTOR_TABLE_SIZE,
				  cfg->tx_hdr, HDR_SIZE,
				  cfg->tx_data, RAMSES_MTU_TC);

	/* XXX check which of these we need */
#if 1
	grspw2_set_promiscuous(&cfg->spw);
#endif
#if 0
	grspw2_protocol_id_drop_enable(&cfg->spw, HDR_PROTO_BYTE, HDR_PROTO_ID);
#endif
}


static void spw_alloc_camera(struct spw_user_cfg *cfg)
{
	cfg->tx_data = (uint8_t *) kpcalloc(1, GRSPW2_TX_DESCRIPTORS * GRSPW2_DEFAULT_MTU);

	cfg->tx_hdr = (uint8_t *) kpcalloc(1, GRSPW2_TX_DESCRIPTORS * HDR_SIZE);
}
/**
 * @brief perform basic initialisation of the spw core
 */


static void spw_init_core_camera(struct spw_user_cfg *cfg)
{
	ramses_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW1);

	/* configure for spw core1 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_1,
			 RAMSES_DPU_ADDR_TO_CAM, SPW_CLCKDIV_START, SPW_CLCKDIV_CAM_RUN,
			 GRSPW2_CAM_RX_MTU, GRSPW2_IRQ_CORE1,
			 GR712_IRL1_AHBSTAT, 0);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  CAM_RX_NDESC * GRSPW2_RX_DESC_SIZE,
				  cfg->rx_data,
				  GRSPW2_CAM_RX_MTU);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  CAM_TX_NDESC * GRSPW2_TX_DESC_SIZE,
				  cfg->tx_hdr, HDR_SIZE,
				  cfg->tx_data, GRSPW2_DEFAULT_MTU);

	grspw2_set_promiscuous(&cfg->spw);
}


static int ramses_init(void)
{
	void *addr;


	memset((void *)CAM_SPW_BUF_START, 0, 4 * 1024);

	/* we require 1kiB tables which are also aligned to 1kiB */
	spw_cfg[0].rx_desc = (uint32_t *)(SPW_DESC_START + 1024 * 0);
	spw_cfg[0].tx_desc = (uint32_t *)(SPW_DESC_START + 1024 * 1);
	spw_cfg[1].rx_desc = (uint32_t *)(SPW_DESC_START + 1024 * 2);
	spw_cfg[1].tx_desc = (uint32_t *)(SPW_DESC_START + 1024 * 3);


	/* we assign only the camera RX buffers, everything else is malloc'd */
	spw_cfg[1].rx_data = (uint8_t *)CAM_SPW_BUF_START;

	spw_alloc_obc(&spw_cfg[0]);
	spw_init_core_obc(&spw_cfg[0]);

	grspw2_core_start(&spw_cfg[0].spw, 1, 1);
	grspw2_set_time_rx(&spw_cfg[0].spw);
	grspw2_tick_out_interrupt_enable(&spw_cfg[0].spw);

	spw_alloc_camera(&spw_cfg[1]);
	spw_init_core_camera(&spw_cfg[1]);
	grspw2_core_start(&spw_cfg[1].spw, 1, 1);


#ifdef CONFIG_EMBED_APPLICATION
	/* load RAMSES ASW */
	addr = module_read_embedded("dpm");
	printk(MSG "test executable address is %p\n", addr);
	if (addr)
		application_load(addr, "ASW", KTHREAD_CPU_AFFINITY_NONE, 0, NULL);
#endif /* CONFIG_EMBED_APPLICATION */

	return 0;
}
lvl1_usercall(ramses_init)
