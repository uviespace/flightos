/**
 * @file init/main.c
 *
 * @ingroup main
 * @defgroup main Kernel Main Function
 * 
 * @brief the kernel main function
 *
 * Here we set up the kernel.
 *
 */

#include <generated/autoconf.h>

#include <kernel/init.h>
#include <kernel/module.h>
#include <kernel/application.h>
#include <kernel/ksym.h>
#include <kernel/printk.h>
#include <kernel/kernel.h>
#include <kernel/kthread.h>
#include <kernel/time.h>
#include <kernel/err.h>
#include <kernel/sysctl.h>
#include <modules-image.h>

#include <kernel/string.h>
#include <kernel/kmem.h>

#include <asm/processor.h>

/* for our demo */
#include "xentium_demo.h"
/* arch irq disable/enable */
#include <asm/irqflags.h>

#include <grspw2.h>


#define MSG "MAIN: "


#if defined(GCC_VERSION) && (GCC_VERSION == 30404)
#ifdef __OPTIMIZE__
#warning "Optimisation flags appear to occasionally produce"
#warning "incorrect code for this compiler version. If something doesn't "
#warning "work when it logically should, select different compiler options"
#warning "in make menuconfig. You have been warned."
#endif /* __OPTIMIZE__ */
#endif /* GCC_VERSION */

/** XXX dummy **/
extern int cpu_ready[CONFIG_SMP_CPUS_MAX];

int copybench_start(void);
int oneshotedf_start(void);




/* a spacewire core configuration (0 = obc,  1 = fee */
struct spw_user_cfg spw_cfg[2];


/* default dividers for GR712RC eval board: 10 Mbit start, 100 Mbit run */
/* XXX: check if we have to set the link start/run speeds to 10 Mbit for
 * both states initally, and reconfigure later, as there will be a RMAP command
 * for the RDCU configuring it to 100 Mbit
 */
#define SPW_CLCKDIV_START	10
#define SPW_CLCKDIV_RUN		5
#define GR712_IRL2_GRSPW2_0	22
#define GR712_IRL2_GRSPW2_1	23
#define GR712_IRL2_GRSPW2_2	24
#define GR712_IRL2_GRSPW2_3	25
#define GR712_IRL2_GRSPW2_4	26
#define GR712_IRL2_GRSPW2_5	27
#define GR712_IRL1_AHBSTAT	1
#define HDR_SIZE	0x4
#define STRIP_HDR_BYTES	0x4

#define SMILE_DPU_ADDR_TO_OBC	0xFE

#define SMILE_DPU_ADDR_TO_FEE    0x50

void smile_set_gr712_spw_clock(void)
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

	mem = (uint32_t) kcalloc(1, GRSPW2_DESCRIPTOR_TABLE_SIZE
				+ GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);

	cfg->rx_desc = (uint32_t *)
		       ((mem + GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN)
			& ~GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);


	mem = (uint32_t) kcalloc(1, GRSPW2_DESCRIPTOR_TABLE_SIZE
				+ GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);

	cfg->tx_desc = (uint32_t *)
		       ((mem + GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN)
			& ~GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);


	/* malloc rx and tx data buffers: decriptors * packet size */
	cfg->rx_data = (uint8_t *) kcalloc(1, GRSPW2_RX_DESCRIPTORS
					  * GRSPW2_DEFAULT_MTU);
	cfg->tx_data = (uint8_t *) kcalloc(1, GRSPW2_TX_DESCRIPTORS
					  * GRSPW2_DEFAULT_MTU);

	cfg->tx_hdr = (uint8_t *) kcalloc(1, GRSPW2_TX_DESCRIPTORS * HDR_SIZE);
}


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



/**
 * @brief perform basic initialisation of the spw core
 */

static void spw_init_core_obc(struct spw_user_cfg *cfg)
{
	smile_set_gr712_spw_clock();

	gr712_clkgate_enable(CLKGATE_GRSPW0);

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_0,
			 SMILE_DPU_ADDR_TO_OBC, SPW_CLCKDIV_START, SPW_CLCKDIV_RUN,
			 GRSPW2_DEFAULT_MTU, GR712_IRL2_GRSPW2_0,
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

	mem = (uint32_t) kcalloc(1, GRSPW2_DESCRIPTOR_TABLE_SIZE
				+ GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);

	cfg->rx_desc = (uint32_t *)
		       ((mem + GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN)
			& ~GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);


	mem = (uint32_t) kcalloc(1, GRSPW2_DESCRIPTOR_TABLE_SIZE
				+ GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);

	cfg->tx_desc = (uint32_t *)
		       ((mem + GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN)
			& ~GRSPW2_DESCRIPTOR_TABLE_MEM_BLOCK_ALIGN);


	/* malloc rx and tx data buffers: decriptors * packet size */
	cfg->rx_data = (uint8_t *) kcalloc(1, GRSPW2_RX_DESCRIPTORS
					  * GRSPW2_DEFAULT_MTU);
	cfg->tx_data = (uint8_t *) kcalloc(1, GRSPW2_TX_DESCRIPTORS
					  * GRSPW2_DEFAULT_MTU);

	cfg->tx_hdr = (uint8_t *) kcalloc(1, GRSPW2_TX_DESCRIPTORS * HDR_SIZE);
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
			 SMILE_DPU_ADDR_TO_FEE, SPW_CLCKDIV_START, SPW_CLCKDIV_RUN,
			 GRSPW2_DEFAULT_MTU, GR712_IRL2_GRSPW2_1,
			 GR712_IRL1_AHBSTAT, 0);

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
	grspw2_set_promiscuous(&cfg->spw);
}





/**
 * @brief kernel initialisation routines
 */

static int kernel_init(void)
{
	setup_arch();

	/* free_bootmem() */
	/* run_init_process() */

	return 0;
}
arch_initcall(kernel_init);


/**
 * @brief kernel main functionputchar( *((char *) data) );
 */
int kernel_main(void)
{
	int i;

	void *addr __attribute__((unused));
	struct elf_binary m __attribute__((unused));




#ifdef CONFIG_MPPB
	/* The mppbv2 LEON's cache would really benefit from cache sniffing...
	 * Interactions with DMA or Xentiums are a pain when using the lower
	 * half of the AHB SDRAM memory schedulebank and since we don't create
	 * a complete memory configuration for this demonstrator, we'll
	 * to just disable the dcache entirely >:(
	 */
	cpu_dcache_disable();
#endif

#ifdef CONFIG_XENTIUM_PROC_DEMO
	/* run the demo */
	xen_demo();
#endif



	/* elevate boot thread */
	kthread_init_main();

	/* wait for cpus */

	for (i = 1; i < CONFIG_SMP_CPUS_MAX; i++) {
printk("waiting for cpu %d, flag at %d\n", i, cpu_ready[i]);
		cpu_ready[i] = 2;
		while (ioread32be(&cpu_ready[i]) != 0x3);
		iowrite32be(0x4, &cpu_ready[i]);

	}

	printk(MSG "Boot complete\n");



#ifdef CONFIG_EMBED_MODULES_IMAGE
	printk(MSG "Loading module image\n");

	/* load the embedded AR image */
	module_image_load_embedded();

	/* look up a kernel symbol */
	printk(MSG "Looked up symbol %s at %p\n",
	       "printk", lookup_symbol("printk"));

	/* look up a file in the embedded image */
	printk(MSG "Looked up file %s at %p\n", "testmodule.ko",
	       module_lookup_embedded("testmodule.ko"));

#if 0
	/* If you have kernel modules embedded in your modules.images, this
	 * is how you can access them:
	 * lookup the module containing <symbol>
	 */
	addr = module_lookup_symbol_embedded("somefunction");
#endif
#if 1
	/* XXX the image is not necessary aligned properly, so we can't access
	 * it directly, until we have a MNA trap */
	addr = module_read_embedded("testmodule.ko");

	pr_debug(MSG "noc_dma module address is %p\n", addr);
	if (addr)
		module_load(&m, addr);
#endif
#if 0
	modules_list_loaded();
#endif

#endif /* CONFIG_EMBED_MODULES_IMAGE */

	/* XXX MEH, just hack this in for EMC test */
	spw_alloc_obc(&spw_cfg[0]);
	spw_init_core_obc(&spw_cfg[0]);

	grspw2_core_start(&spw_cfg[0].spw);
	grspw2_set_time_rx(&spw_cfg[0].spw);
	grspw2_tick_out_interrupt_enable(&spw_cfg[0].spw);

	spw_alloc_fee(&spw_cfg[1]);
	spw_init_core_fee(&spw_cfg[1]);
	grspw2_core_start(&spw_cfg[1].spw);

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
#if 0
	/* dummy demonstrator */
	addr = module_read_embedded("executable_demo");
#else
#if 0
	/* CrIa */
	addr = module_read_embedded("myapp");
#endif
#endif
#if 0
	addr = (void *) 0x60000000;
	pr_info(MSG "test executable address is %p\n", addr);
	if (addr)
		module_load(&m, addr);

#endif
#if 0
	modules_list_loaded();
#endif


#endif	/* CONFIG_EMBED_APPLICATION */

#if 0
	oneshotedf_start();
#endif
#if 0
	while (1) {

		if (grspw2_get_num_pkts_avail(&spw_cfg.spw)) {  
			int32_t elem;
			int8_t *buf;
			int i;

			printk("packet!\n");

			elem = grspw2_get_next_pkt_size(&spw_cfg.spw);

			buf = kmalloc(elem);
			BUG_ON(!buf);

			elem = grspw2_get_pkt(&spw_cfg.spw, buf);
			BUG_ON(!elem);

			for (i = 0; i < elem; i++)
				printk("%02X:", buf[i]);

			printk("\n");


			kfree(buf);


		}
	
		cpu_relax();
	}
#endif


	/* CrIa */
	addr = module_read_embedded("CrIa");
	printk(MSG "test executable address is %p\n", addr);
#if 1
	if (addr)
		application_load( (void*) addr, "CrIa", KTHREAD_CPU_AFFINITY_NONE);
#endif

	{
		struct timespec t = get_uptime();
		printk("entering main idle loop at uptime %u s, %u ms\n", t.tv_sec, t.tv_nsec / 1000000);
		MARK();
	}
	while(1) {
#if 0
		printk("link status is %x and %x\n", grspw2_get_link_status(&spw_cfg[0].spw), grspw2_get_link_status(&spw_cfg[1].spw) );
#endif
		cpu_relax();
	}


	/* never reached */
	BUG();

	return 0;
}

#ifdef CONFIG_ARCH_CUSTOM_BOOT_CODE

extern initcall_t __initcall_start;
extern initcall_t __initcall_end;

static void do_initcalls(void)
{
    initcall_t *p = &__initcall_start;

    while(p < &__initcall_end) {
        if (*p)
            (*p)();
	p++;
    }

}

void do_basic_setup(void)
{
	do_initcalls();
}

#else

int main(void)
{
	return kernel_main();
}

#endif /* CONFIG_ARCH_CUSTOM_BOOT_CODE */



