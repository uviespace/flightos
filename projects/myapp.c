#include <kernel/init.h>
#include <kernel/kmem.h>
#include <kernel/kthread.h>
#include <kernel/module.h>
#include <kernel/application.h>
#include <kernel/user.h>
#include <kernel/signals.h>
#include <asm-generic/io.h>

#include <grspw2.h>
#include <modules-image.h>

#define MSG "MYAPP: "


struct spw_user_cfg spw_cfg[2];

/* default dividers for GR712RC eval board: 10 Mbit start, 100 Mbit run */
/* XXX: check if we have to set the link start/run speeds to 10 Mbit for
 * both states initally, and reconfigure later, as there will be a RMAP command
 * for the RDCU configuring it to 100 Mbit
 */
#define SPW_CLCKDIV_START	10
#define SPW_CLCKDIV_PLM_RUN	1
#define GR712_IRL1_AHBSTAT	1

#define HDR_SIZE		0x4
#define STRIP_HDR_BYTES		0x4

#define HDR_PROTO_BYTE		0x1
#define HDR_PROTO_ID		0x2

#define DPU_ADDR_TO_OBC		0xFE




#define ARIEL_DPU_ADDR_TO_OBC	0x53

#define ARIEL_MTU_DCU		(32 * 1024)
#define ARIEL_MTU_TM		ARIEL_MTU_DCU			/* no idea, just took this from BSW ICD */
#define ARIEL_MTU_TC		ARIEL_MTU_DCU	/* Table 1.0, ARIEL-SPW-858 according to BSW ICD + 4byte header */



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


static void myapp_set_gr712_spw_clock(void)
{
	uint32_t *gpreg = (uint32_t *) 0x80000600;


	(*gpreg) = (ioread32be(gpreg) & (0xFFFFFFF8));
}



static void spw_alloc_desc_table(struct spw_user_cfg *cfg, size_t tc_size, size_t tm_size, size_t hdr_size, size_t rx_desc, size_t tx_desc)
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
	cfg->rx_data = (uint8_t *) kpcalloc(1, rx_desc * tc_size);
	cfg->tx_data = (uint8_t *) kpcalloc(1, tx_desc * tm_size);

	cfg->tx_hdr = (uint8_t *) kpcalloc(1, tx_desc * hdr_size);
}






/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_cam(struct spw_user_cfg *cfg)
{
	myapp_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW0);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_0,
			 ARIEL_DPU_ADDR_TO_DCU, SPW_CLCKDIV_START, SPW_CLCKDIV_PLM_RUN,
			 ARIEL_MTU_TC, GRSPW2_IRQ_CORE0,
			 GR712_IRL1_AHBSTAT, 0);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  GRSPW2_RX_DESC_SIZE * 5,
				  cfg->rx_data,
				  ARIEL_MTU_DCU);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  GRSPW2_TX_DESC_SIZE *  5,
				  cfg->tx_hdr, HDR_SIZE,
				  cfg->tx_data, ARIEL_MTU_DCU);
}



/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_debug(struct spw_user_cfg *cfg)
{
	myapp_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW1);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_1,
			 ARIEL_DPU_ADDR_TO_DEBUG, SPW_CLCKDIV_START, SPW_CLCKDIV_PLM_RUN,
			 ARIEL_MTU_TC, GRSPW2_IRQ_CORE1,
			 GR712_IRL1_AHBSTAT, 0);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  GRSPW2_RX_DESC_SIZE * 64,
				  cfg->rx_data,
				  ARIEL_MTU_DCU);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  GRSPW2_TX_DESC_SIZE * 64,
				  cfg->tx_hdr, 0,
				  cfg->tx_data, ARIEL_MTU_DCU);

}






__attribute__((unused))
static int sigtest(void *data)
{
	struct timespec ts;
	ktime wake;


	wake = ktime_add_ms(ktime_get(), 1000);
	while (1) {
		if (ktime_after(ktime_get(), wake)) {

			ksignal_send_info(12, NULL);

			wake = ktime_add_ms(ktime_get(), 2000);
		}
	}

	return 0;
}


static int myapp_init(void)
{
#if 0
	struct task_struct *t = kthread_create(sigtest, NULL, 0, "SIG");

	kthread_set_sched_edf(t, 10 * 1000, 9 * 1000, 500);

	if (kthread_wake_up(t) < 0) {
		printk("---- %s NOT SCHEDUL-ABLE---\n", t->name);
		BUG();
	}
#endif
if (0) {
	/* setup routing between dcu and debug link 5 */
	spw_alloc_desc_table(&spw_cfg[0], ARIEL_MTU_DCU, ARIEL_MTU_DCU, 0, 64, 64);
	spw_alloc_desc_table(&spw_cfg[1], ARIEL_MTU_DCU, ARIEL_MTU_DCU, 0, 64, 64);

	spw_init_core_cam(&spw_cfg[0]);
	spw_init_core_debug(&spw_cfg[1]);

	grspw2_core_start(&spw_cfg[0].spw, 1, 1);
	grspw2_core_start(&spw_cfg[1].spw, 1, 1);

	grspw2_enable_routing(&spw_cfg[0].spw, &spw_cfg[1].spw);
	grspw2_enable_routing(&spw_cfg[1].spw, &spw_cfg[0].spw);
}

	if (CONFIG_EMBED_APPLICATION) {
		/* load SMILE ASW */
		void *addr = module_read_embedded("myapp");
		printk(MSG "test executable address is %p\n", addr);
		if (addr)
			application_load(addr, "ASW", KTHREAD_CPU_AFFINITY_NONE, 0, NULL);
	}
	return 0;
}
lvl1_usercall(myapp_init)
