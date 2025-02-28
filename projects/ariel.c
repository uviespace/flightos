
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

#define MSG "ARIEL: "


/* a spacewire core configuration (0 = obc,  1...N = ...TODO... */
struct spw_user_cfg spw_cfg[6];

#define SPW_CLCKDIV_START	6
#define SPW_CLCKDIV_PLM_RUN	1
#define SPW_CLCKDIV_FEE_RUN	2
#define GR712_IRL1_AHBSTAT	1

#define HDR_SIZE		0x4
#define STRIP_HDR_BYTES		0x4

#define ARIEL_DPU_ADDR_TO_OBC	0x53

#define ARIEL_MTU_TM		4096			/* no idea, just took this from BSW ICD */
#define ARIEL_MTU_TC		GRSPW2_DEFAULT_MTU	/* Table 1.0, ARIEL-SPW-858 according to BSW ICD + 4byte header */

#define ARIEL_MTU_DCU		(32 * 1024)


#define ARIEL_DPU_ADDR_TO_DEBUG	0x66	/* debug link 5, used for routing to DCU */
#define ARIEL_DPU_ADDR_TO_DCU	0x77	/* link 3, used for routing to DCU XXX fix address (maybew not needed due to promisc routing mode */



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


static void ariel_set_gr712_spw_clock(void)
{
	uint32_t *gpreg = (uint32_t *) 0x80000600;


	(*gpreg) = (ioread32be(gpreg) & (0xFFFFFFF8));
}


static void spw_alloc_desc_table(struct spw_user_cfg *cfg, size_t tc_size, size_t tm_size, size_t hdr_size)
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
	cfg->rx_data = (uint8_t *) kpcalloc(1, GRSPW2_RX_DESCRIPTORS * tc_size);
	cfg->tx_data = (uint8_t *) kpcalloc(1, GRSPW2_TX_DESCRIPTORS * tm_size);

	cfg->tx_hdr = (uint8_t *) kpcalloc(1, GRSPW2_TX_DESCRIPTORS * hdr_size);
}




/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_obc(struct spw_user_cfg *cfg)
{
	ariel_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW0);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_0,
			 ARIEL_DPU_ADDR_TO_OBC, SPW_CLCKDIV_START, SPW_CLCKDIV_PLM_RUN,
			 ARIEL_MTU_TC, GRSPW2_IRQ_CORE0,
			 GR712_IRL1_AHBSTAT, STRIP_HDR_BYTES);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  GRSPW2_DESCRIPTOR_TABLE_SIZE,
				  cfg->rx_data,
				  ARIEL_MTU_TC);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  GRSPW2_DESCRIPTOR_TABLE_SIZE,
				  cfg->tx_hdr, HDR_SIZE,
				  cfg->tx_data, ARIEL_MTU_TM);
}



/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_dcu(struct spw_user_cfg *cfg)
{
	ariel_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW5);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_3,
			 ARIEL_DPU_ADDR_TO_DCU, SPW_CLCKDIV_START, SPW_CLCKDIV_PLM_RUN,
			 ARIEL_MTU_TC, GRSPW2_IRQ_CORE3,
			 GR712_IRL1_AHBSTAT, 0);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  GRSPW2_DESCRIPTOR_TABLE_SIZE,
				  cfg->rx_data,
				  ARIEL_MTU_DCU);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  GRSPW2_DESCRIPTOR_TABLE_SIZE,
				  cfg->tx_hdr, 0,
				  cfg->tx_data, ARIEL_MTU_DCU);
}



/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_debug(struct spw_user_cfg *cfg)
{
	ariel_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW5);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_5,
			 ARIEL_DPU_ADDR_TO_DEBUG, SPW_CLCKDIV_START, SPW_CLCKDIV_PLM_RUN,
			 ARIEL_MTU_TC, GRSPW2_IRQ_CORE5,
			 GR712_IRL1_AHBSTAT, 0);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  GRSPW2_DESCRIPTOR_TABLE_SIZE,
				  cfg->rx_data,
				  ARIEL_MTU_DCU);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  GRSPW2_DESCRIPTOR_TABLE_SIZE,
				  cfg->tx_hdr, 0,
				  cfg->tx_data, ARIEL_MTU_DCU);
}














__attribute__((unused))
static void ariel_edac_reset(void *data)
{
#if 0
	/* overwrite dbit error */
	iowrite32be(0, (void *) 0x61000000);
#endif
}


__attribute__((unused))
static void ariel_check_edac(void)
{
#if 0
	edac_set_reset_callback(ariel_edac_reset, NULL);

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
#endif
}


static int ariel_init(void)
{
	void *addr;


	spw_alloc_desc_table(&spw_cfg[0], ARIEL_MTU_TC, ARIEL_MTU_TM, HDR_SIZE);
	spw_init_core_obc(&spw_cfg[0]);


	grspw2_core_start(&spw_cfg[0].spw, 1, 1);
	grspw2_set_time_rx(&spw_cfg[0].spw);
	grspw2_tick_out_interrupt_enable(&spw_cfg[0].spw);


	/* setup routing between dcu and debug link 5 */
	spw_alloc_desc_table(&spw_cfg[2], ARIEL_MTU_DCU, ARIEL_MTU_DCU, 0);
	spw_alloc_desc_table(&spw_cfg[4], ARIEL_MTU_DCU, ARIEL_MTU_DCU, 0);

	spw_init_core_dcu(&spw_cfg[2]);
	spw_init_core_debug(&spw_cfg[4]);

	grspw2_core_start(&spw_cfg[2].spw, 1, 1);
	grspw2_core_start(&spw_cfg[4].spw, 1, 1);

	grspw2_enable_routing(&spw_cfg[2].spw, &spw_cfg[4].spw);
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
	/* load ARIEL ASW */
	addr = module_read_embedded("CrIa");
	printk(MSG "test executable address is %p\n", addr);
	if (addr)
		application_load(addr, "ASW", KTHREAD_CPU_AFFINITY_NONE, 0, NULL);
#endif /* CONFIG_EMBED_APPLICATION */

	return 0;
}
lvl1_usercall(ariel_init)
