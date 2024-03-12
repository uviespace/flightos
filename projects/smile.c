
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

#define MSG "SMILE: "


/* a spacewire core configuration (0 = obc,  1 = fee */
struct spw_user_cfg spw_cfg[2];

/* default dividers for GR712RC eval board: 10 Mbit start, 100 Mbit run */
/* XXX: check if we have to set the link start/run speeds to 10 Mbit for
 * both states initally, and reconfigure later, as there will be a RMAP command
 * for the RDCU configuring it to 100 Mbit
 */
#define SPW_CLCKDIV_START	10
#define SPW_CLCKDIV_PLM_RUN	10
#define SPW_CLCKDIV_FEE_RUN	2
#define GR712_IRL1_AHBSTAT	1

#define HDR_SIZE		0x4
#define STRIP_HDR_BYTES		0x4

#define SMILE_DPU_ADDR_TO_OBC	0xFE
#define SMILE_DPU_ADDR_TO_FEE   0x50


#define CLKGATE_GRETH		0x00000001
#define CLKGATE_GRSPW0		0x00000002
#define CLKGATE_GRSPW1		0x00000004
#define CLKGATE_GRSPW2		0x00000008
#define CLKGATE_GRSPW3		0x00000010
#define CLKGATE_GRSPW4		0x00000020
#define CLKGATE_GRSPW5		0x00000040
#define CLKGATE_CAN		0x00000080

/* bit 8 is proprietary */
#define CLKGATE_CCSDS_TM	0x00000200
#define CLKGATE_CCSDS_TC	0x00000400
#define CLKGATE_1553BRM		0x00000800

#define CLKGATE_BASE		0x80000D00

__attribute__((unused))
	static struct gr712_clkgate {
		uint32_t unlock;
		uint32_t clk_enable;
		uint32_t core_reset;
	} *clkgate = (struct gr712_clkgate *) CLKGATE_BASE;


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


static void smile_set_gr712_spw_clock(void)
{
	uint32_t *gpreg = (uint32_t *) 0x80000600;


	(*gpreg) = (ioread32be(gpreg) & (0xFFFFFFF8));
}


static void spw_alloc_obc(struct spw_user_cfg *cfg)
{
	uint32_t mem;


	/*
	 * malloc a rx and tx descriptor table buffer and align to
	 * 1024 bytes (GR712UMRC, p. 111)
	 *
	 * dynamically allocate memory + 1K for alignment (worst case)
	 * 1 buffer per dma channel (GR712 cores only implement one channel)
	 *
	 * NOTE: we don't care about calling free(), because this is a
	 * bare-metal demo, so we just discard the original pointer
	 */

	mem = (uint32_t) kpcalloc(1, GRSPW2_DESCRIPTOR_TABLE_SIZE
				  + GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);

	cfg->rx_desc = (uint32_t *)
		((mem + GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN)
		 & ~GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);


	mem = (uint32_t) kpcalloc(1, GRSPW2_DESCRIPTOR_TABLE_SIZE
				  + GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);

	cfg->tx_desc = (uint32_t *)
		((mem + GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN)
		 & ~GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);


	/* malloc rx and tx data buffers: decriptors * packet size */
	cfg->rx_data = (uint8_t *) kpcalloc(1, GRSPW2_RX_DESCRIPTORS
					    * GRSPW2_DEFAULT_MTU);
	cfg->tx_data = (uint8_t *) kpcalloc(1, GRSPW2_TX_DESCRIPTORS
					    * GRSPW2_DEFAULT_MTU);

	cfg->tx_hdr = (uint8_t *) kpcalloc(1, GRSPW2_TX_DESCRIPTORS * HDR_SIZE);
}




/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_obc(struct spw_user_cfg *cfg)
{
	smile_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW0);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_0,
			 SMILE_DPU_ADDR_TO_OBC, SPW_CLCKDIV_START, SPW_CLCKDIV_PLM_RUN,
			 GRSPW2_DEFAULT_MTU, GRSPW2_IRQ_CORE0,
			 GR712_IRL1_AHBSTAT, STRIP_HDR_BYTES);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  GRSPW2_DESCRIPTOR_TABLE_SIZE,
				  cfg->rx_data,
				  GRSPW2_DEFAULT_MTU);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  GRSPW2_DESCRIPTOR_TABLE_SIZE,
				  cfg->tx_hdr, HDR_SIZE,
				  cfg->tx_data, GRSPW2_DEFAULT_MTU);
}


/* the SMILE FEE does not always honour the packet size settings,
 * so we increase the RX size
 */
#define GRSPW2_FEE_RX_MTU	0x4004
static void spw_alloc_fee(struct spw_user_cfg *cfg)
{
	uint32_t mem;


	/*
	 * malloc a rx and tx descriptor table buffer and align to
	 * 1024 bytes (GR712UMRC, p. 111)
	 *
	 * dynamically allocate memory + 1K for alignment (worst case)
	 * 1 buffer per dma channel (GR712 cores only implement one channel)
	 *
	 * NOTE: we don't care about calling free(), because this is a
	 * bare-metal demo, so we just discard the original pointer
	 */

	mem = (uint32_t) kpcalloc(1, GRSPW2_DESCRIPTOR_TABLE_SIZE
				  + GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);

	cfg->rx_desc = (uint32_t *)
		((mem + GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN)
		 & ~GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);


	mem = (uint32_t) kpcalloc(1, GRSPW2_DESCRIPTOR_TABLE_SIZE
				  + GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);

	cfg->tx_desc = (uint32_t *)
		((mem + GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN)
		 & ~GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);


	/* malloc rx and tx data buffers: decriptors * packet size */
	cfg->rx_data = (uint8_t *) kpcalloc(1, GRSPW2_RX_DESCRIPTORS
					    * GRSPW2_FEE_RX_MTU);
	cfg->tx_data = (uint8_t *) kpcalloc(1, GRSPW2_TX_DESCRIPTORS
					    * GRSPW2_DEFAULT_MTU);

	cfg->tx_hdr = (uint8_t *) kpcalloc(1, GRSPW2_TX_DESCRIPTORS * HDR_SIZE);
}
/**
 * @brief perform basic initialisation of the spw core
 */


static void spw_init_core_fee(struct spw_user_cfg *cfg)
{
	smile_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW1);

	/* configure for spw core1 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_1,
			 SMILE_DPU_ADDR_TO_FEE, SPW_CLCKDIV_START, SPW_CLCKDIV_FEE_RUN,
			 GRSPW2_FEE_RX_MTU, GRSPW2_IRQ_CORE1,
			 GR712_IRL1_AHBSTAT, 0);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  GRSPW2_DESCRIPTOR_TABLE_SIZE,
				  cfg->rx_data,
				  GRSPW2_FEE_RX_MTU);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  GRSPW2_DESCRIPTOR_TABLE_SIZE,
				  cfg->tx_hdr, HDR_SIZE,
				  cfg->tx_data, GRSPW2_DEFAULT_MTU);
	grspw2_set_promiscuous(&cfg->spw);
}


__attribute__((unused))
static void smile_edac_reset(void *data)
{
	/* overwrite dbit error */
	iowrite32be(0, (void *) 0x61000000);
}


__attribute__((unused))
static void smile_check_edac(void)
{
	edac_set_reset_callback(smile_edac_reset, NULL);

	sysset_show_tree(NULL);

	edac_critical_segment_add((void *) 0x61000000, (void *) 0x61000004);
	edac_inject_fault((void *) 0x61000000, 0x0, 0x1);


	{
		char buf[256];
		sysobj_show_attr(sysset_find_obj(NULL, "/sys/edac/"), "singlefaults", buf);
		printk("single: %s\n", buf);
		sysobj_show_attr(sysset_find_obj(NULL, "/sys/edac/"), "lastsingleaddr", buf);
		printk("last: %s\n", buf);
	}


	printk("I read %d\n", ioread32be((void *) 0x61000000));
	{
		char buf[256];
		sysobj_show_attr(sysset_find_obj(NULL, "/sys/edac/"), "singlefaults", buf);
		printk("single: %s\n", buf);
		sysobj_show_attr(sysset_find_obj(NULL, "/sys/edac/"), "lastsingleaddr", buf);
		printk("last: %s\n", buf);
	}


	edac_inject_fault((void *) 0x61000000, 0x0, 0x3);
	printk("I read %d\n", ioread32be((void *) 0x61000000));
	{
		char buf[256];
		sysobj_show_attr(sysset_find_obj(NULL, "/sys/edac/"), "doublefaults", buf);
		printk("single: %s\n", buf);
	}

	memscrub_seg_add(0x60000000, 0x62000000, 256);
}


static int smile_init(void)
{
	void *addr;


	/* XXX MEH, just hack this in for EMC test */
	spw_alloc_obc(&spw_cfg[0]);
	spw_init_core_obc(&spw_cfg[0]);

	grspw2_core_start(&spw_cfg[0].spw, 0, 1);
	grspw2_set_time_rx(&spw_cfg[0].spw);
	grspw2_tick_out_interrupt_enable(&spw_cfg[0].spw);

	spw_alloc_fee(&spw_cfg[1]);
	spw_init_core_fee(&spw_cfg[1]);
	grspw2_core_start(&spw_cfg[1].spw, 1, 1);

#if 0
	/* empty link in case the mkII brick acts up again,
	 * note that this does not work unless the power to the FEE psu
	 * is ON for the SXI DPU, as the LVDS transceivers are not
	 * powered otherwise; this is a devel workaround anyways
	 */
	iowrite32be((1 << 12), (void *) 0x20000420);
	iowrite32be(0x9A000000, (void *) 0x20000428);

	printk("status is %08x\n", ioread32be((void *) 0x2000042C) );

	while (grspw2_get_num_pkts_avail(&spw_cfg[1].spw)) {
		grspw2_drop_pkt(&spw_cfg[1].spw);
		printk(".");
	}
	printk("\n");
#endif


#ifdef CONFIG_EMBED_APPLICATION
	/* load SMILE ASW */
	addr = module_read_embedded("CrIa");
	printk(MSG "test executable address is %p\n", addr);
	if (addr)
		application_load(addr, "ASW", KTHREAD_CPU_AFFINITY_NONE, 0, NULL);
#endif /* CONFIG_EMBED_APPLICATION */

	return 0;
}
lvl1_usercall(smile_init)
