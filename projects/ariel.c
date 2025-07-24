
#include <kernel/init.h>
#include <kernel/kmem.h>
#include <kernel/kthread.h>
#include <kernel/module.h>
#include <kernel/application.h>
#include <kernel/edac.h>
#include <kernel/memscrub.h>
#include <kernel/user.h>
#include <kernel/irq.h>
#include <kernel/signals.h>

#include <grspw2.h>
#include <asm-generic/io.h>
#include <modules-image.h>

#define MSG "ARIEL: "


/* a spacewire core configuration (0 = obc,  1...N = ...TODO... */
struct spw_user_cfg spw_cfg[6];

#define SPW_CLCKDIV_START	6
#define SPW_CLCKDIV_OBC_RUN	1
#define SPW_CLCKDIV_FEE_RUN	2
#define GR712_IRL1_AHBSTAT	1

#define HDR_PROTO_BYTE		0x1
#define HDR_PROTO_ID		0x2

#define HDR_SIZE		0x4
#define STRIP_HDR_BYTES		0x4

#define ARIEL_DPU_ADDR_TO_OBC_NOM	0x52
#define ARIEL_DPU_ADDR_TO_OBC_RED	0x53

#define ARIEL_MTU_TM		4096			/* no idea, just took this from BSW ICD */
#define ARIEL_MTU_TC		GRSPW2_DEFAULT_MTU	/* Table 1.0, ARIEL-SPW-858 according to BSW ICD + 4byte header */

#define ARIEL_MTU_DCU		(32 * 1024)


#define ARIEL_DPU_ADDR_TO_DEBUG	0x66	/* debug link 5, used for routing to DCU */
#define ARIEL_DPU_ADDR_TO_DCU_NOM 0x77  /* XXX unsure if we are supposed to use a specific address here */
#define ARIEL_DPU_ADDR_TO_DCU_RED 0x88



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

static void spw_init_core_obc_nominal(struct spw_user_cfg *cfg, uint32_t n_rx_desc, uint32_t n_tx_desc)
{
	ariel_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW0);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_0,
			 ARIEL_DPU_ADDR_TO_OBC_NOM, SPW_CLCKDIV_START, SPW_CLCKDIV_OBC_RUN,
			 ARIEL_MTU_TC, GRSPW2_IRQ_CORE0,
			 GR712_IRL1_AHBSTAT, STRIP_HDR_BYTES);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  n_rx_desc * GRSPW2_RX_DESC_SIZE,
				  cfg->rx_data,
				  ARIEL_MTU_TC);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  n_tx_desc * GRSPW2_TX_DESC_SIZE,
				  cfg->tx_hdr, HDR_SIZE,
				  cfg->tx_data, ARIEL_MTU_TM);


	grspw2_protocol_id_drop_enable(&cfg->spw, HDR_PROTO_BYTE, HDR_PROTO_ID);
}


/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_obc_redundant(struct spw_user_cfg *cfg, uint32_t n_rx_desc, uint32_t n_tx_desc)
{
	ariel_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW1);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_1,
			 ARIEL_DPU_ADDR_TO_OBC_RED, SPW_CLCKDIV_START, SPW_CLCKDIV_OBC_RUN,
			 ARIEL_MTU_TC, GRSPW2_IRQ_CORE1,
			 GR712_IRL1_AHBSTAT, STRIP_HDR_BYTES);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  n_rx_desc * GRSPW2_RX_DESC_SIZE,
				  cfg->rx_data,
				  ARIEL_MTU_TC);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  n_tx_desc * GRSPW2_TX_DESC_SIZE,
				  cfg->tx_hdr, HDR_SIZE,
				  cfg->tx_data, ARIEL_MTU_TM);


	grspw2_protocol_id_drop_enable(&cfg->spw, HDR_PROTO_BYTE, HDR_PROTO_ID);
}


/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_dcu_nom(struct spw_user_cfg *cfg, uint32_t n_rx_desc, uint32_t n_tx_desc, uint32_t hdr_size)
{
	ariel_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW2);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_2,
			 ARIEL_DPU_ADDR_TO_DCU_NOM, SPW_CLCKDIV_START, SPW_CLCKDIV_OBC_RUN,
			 ARIEL_MTU_TC, GRSPW2_IRQ_CORE2,
			 GR712_IRL1_AHBSTAT, 0);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  n_rx_desc * GRSPW2_RX_DESC_SIZE,
				  cfg->rx_data,
				  ARIEL_MTU_DCU);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  n_tx_desc * GRSPW2_RX_DESC_SIZE,
				  cfg->tx_hdr, hdr_size,
				  cfg->tx_data, ARIEL_MTU_DCU);
}


/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_dcu_red(struct spw_user_cfg *cfg, uint32_t n_rx_desc, uint32_t n_tx_desc, uint32_t hdr_size)
{
	ariel_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW2);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_3,
			 ARIEL_DPU_ADDR_TO_DCU_RED, SPW_CLCKDIV_START, SPW_CLCKDIV_OBC_RUN,
			 ARIEL_MTU_TC, GRSPW2_IRQ_CORE3,
			 GR712_IRL1_AHBSTAT, 0);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  n_rx_desc * GRSPW2_RX_DESC_SIZE,
				  cfg->rx_data,
				  ARIEL_MTU_DCU);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  n_tx_desc * GRSPW2_RX_DESC_SIZE,
				  cfg->tx_hdr, hdr_size,
				  cfg->tx_data, ARIEL_MTU_DCU);
}



/**
 * @brief perform basic initialisation of the spw core
 */
__attribute__((unused))
static void spw_init_core_debug(struct spw_user_cfg *cfg)
{
	ariel_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW4);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_4,
			 ARIEL_DPU_ADDR_TO_DEBUG, SPW_CLCKDIV_START, SPW_CLCKDIV_OBC_RUN,
			 ARIEL_MTU_TC, GRSPW2_IRQ_CORE4,
			 GR712_IRL1_AHBSTAT, 0);

	grspw2_rx_desc_table_init(&cfg->spw,
				  cfg->rx_desc,
				  GRSPW2_RX_DESC_SIZE * 5,
				  cfg->rx_data,
				  ARIEL_MTU_DCU);

	grspw2_tx_desc_table_init(&cfg->spw,
				  cfg->tx_desc,
				  GRSPW2_TX_DESC_SIZE * 5,
				  cfg->tx_hdr, 0,
				  cfg->tx_data, ARIEL_MTU_DCU);

}

/* emit signals 101 and 102 to indicate packet on a DCU link */
static irqreturn_t emit_irq_dcu1(unsigned int irq, void *userdata)
{
	ksignal_send_info(101, NULL);
	return 0;
}

static irqreturn_t emit_irq_dcu2(unsigned int irq, void *userdata)
{
	ksignal_send_info(102, NULL);
	return 0;
}

static int ariel_init(void)
{
	void *addr;


	spw_alloc_desc_table(&spw_cfg[0], ARIEL_MTU_TC, ARIEL_MTU_TM, HDR_SIZE, 8, GRSPW2_TX_DESCRIPTORS);
	spw_init_core_obc_nominal(&spw_cfg[0], 8, GRSPW2_TX_DESCRIPTORS);

	spw_alloc_desc_table(&spw_cfg[1], ARIEL_MTU_TC, ARIEL_MTU_TM, HDR_SIZE, 8, GRSPW2_TX_DESCRIPTORS);
	spw_init_core_obc_redundant(&spw_cfg[1], 8, GRSPW2_TX_DESCRIPTORS);

	grspw2_core_start(&spw_cfg[0].spw, 1, 1);
	grspw2_set_time_rx(&spw_cfg[0].spw);
	grspw2_tick_out_interrupt_enable(&spw_cfg[0].spw);
	grspw2_set_promiscuous(&spw_cfg[0].spw);

	grspw2_core_start(&spw_cfg[1].spw, 1, 1);
	grspw2_set_time_rx(&spw_cfg[1].spw);
	grspw2_tick_out_interrupt_enable(&spw_cfg[1].spw);
	grspw2_set_promiscuous(&spw_cfg[1].spw);


	spw_alloc_desc_table(&spw_cfg[2], ARIEL_MTU_DCU, ARIEL_MTU_DCU, 40, 5, 5);
	spw_init_core_dcu_nom(&spw_cfg[2], 5, 5, 40);
	grspw2_core_start(&spw_cfg[2].spw, 1, 1);
	grspw2_set_promiscuous(&spw_cfg[2].spw);
	grspw2_rx_interrupt_enable(&spw_cfg[2].spw);
	irq_request(spw_cfg[2].spw.core_irq, ISR_PRIORITY_NOW, emit_irq_dcu1, NULL);

	spw_alloc_desc_table(&spw_cfg[3], ARIEL_MTU_DCU, ARIEL_MTU_DCU, 40, 5, 5);
	spw_init_core_dcu_red(&spw_cfg[3], 5, 5, 40);
	grspw2_core_start(&spw_cfg[3].spw, 1, 1);
	grspw2_set_promiscuous(&spw_cfg[3].spw);
	grspw2_rx_interrupt_enable(&spw_cfg[3].spw);
	irq_request(spw_cfg[3].spw.core_irq, ISR_PRIORITY_NOW, emit_irq_dcu2, NULL);


	if (CONFIG_EMBED_APPLICATION) {
		/* load ARIEL ASW */
		addr = module_read_embedded("asw");
		printk(MSG "test executable address is %p\n", addr);
		if (addr)
			application_load(addr, "ASW", KTHREAD_CPU_AFFINITY_NONE, 0, NULL);
	}

	return 0;
}
lvl1_usercall(ariel_init)
