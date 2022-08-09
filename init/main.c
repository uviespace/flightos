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
#include <kernel/syscalls.h>

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


SYSCALL_DEFINE2(read, int, a, char *, s)
{
	printk("SYSCALL READ int %d, string %s\n", a, s);
	return a * a;
}

SYSCALL_DEFINE3(write, int, a, char *, s, char *, s2)
{
	printk("SYSCALL WRITE int %d, string %s, other string %s\n", a, s, s2);
	return a * a;
}

long sys_ni_syscall(int a, char *s)
{
	printk("NINI\n");
	return 0;
}


SYSCALL_DEFINE2(mem_alloc, size_t, size, void **, p)
{
	(*p) = kmalloc(size);

	if (!(*p))
		return -ENOMEM;

	return 0;
}

SYSCALL_DEFINE1(mem_free, void *, p)
{
	kfree(p);

	return 0;
}

SYSCALL_DEFINE1(clock_gettime, struct timespec *, t)
{
	struct timespec kt;


	if (!t)
		return -EINVAL;

	kt = get_ktime();

	memcpy(t, &kt, sizeof(struct timespec));

	return 0;
}


SYSCALL_DEFINE2(clock_nanosleep, int, flags, struct timespec *, t)
{
#define TIMER_ABSTIME   4
	struct timespec ts;
	ktime wake;

	if (!t)
		return -EINVAL;

	memcpy(&ts, t, sizeof(struct timespec));

	if (flags & TIMER_ABSTIME)
		wake = timespec_to_ktime(ts);
	else
		wake = ktime_add(ktime_get(), timespec_to_ktime(ts));

#if 0
	printk("sleep for %g ms\n", 0.001 * (double)  ktime_ms_delta(wake, ktime_get()));
#endif
	/* just busy-wait for now */
	while (ktime_after(wake, ktime_get()));

	return 0;
}






#undef __SYSCALL
#define __SYSCALL(nr, sym)	[nr] = sym,

#define __NR_syscalls  (10)	/* XXX update in ASM manually!! (dummy) */

/*
 * The sys_call_table array must be 4K aligned to be accessible from
 * kernel/entry.S.
 */
void *syscall_tbl[__NR_syscalls] __aligned(4096) = {
	[0 ... __NR_syscalls - 1] = sys_ni_syscall,
/*#include <asm/unistd.h>*/
	__SYSCALL(0,   sys_read)
	__SYSCALL(1,   sys_write)
	__SYSCALL(2,   sys_mem_alloc)
	__SYSCALL(3,   sys_mem_free)
	__SYSCALL(4,   sys_clock_gettime)
	__SYSCALL(5,   sys_clock_nanosleep)
};


/* a spacewire core configuration */
static struct spw_cfg {
	struct grspw2_core_cfg spw;
	uint32_t *rx_desc;
	uint32_t *tx_desc;
	uint8_t  *rx_data;
	uint8_t  *tx_data;
	uint8_t  *tx_hdr;
} spw_cfg;


/* default dividers for GR712RC eval board: 10 Mbit start, 100 Mbit run */
/* XXX: check if we have to set the link start/run speeds to 10 Mbit for
 * both states initally, and reconfigure later, as there will be a RMAP command
 * for the RDCU configuring it to 100 Mbit
 */
#define SPW_CLCKDIV_START	10
#define SPW_CLCKDIV_RUN		1
#define GR712_IRL2_GRSPW2_0	22	
#define GR712_IRL1_AHBSTAT	1
#define HDR_SIZE	0x0

#define ICU_ADDR	0x28	/* PLATO-IWF-PL-TR-0042-d0-1_RDCU_Functional_Test_PT1 */


static void spw_alloc(struct spw_cfg *cfg)
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

static void spw_init_core(struct spw_cfg *cfg)
{
	/* select GR712 INCLCK */
	set_gr712_spw_clock();

	/* configure for spw core0 */
	grspw2_core_init(&cfg->spw, GRSPW2_BASE_CORE_0,
			 ICU_ADDR, SPW_CLCKDIV_START, SPW_CLCKDIV_RUN,
			 GRSPW2_DEFAULT_MTU, GR712_IRL2_GRSPW2_0,
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


	spw_alloc(&spw_cfg);
	spw_init_core(&spw_cfg);

	grspw2_core_start(&spw_cfg.spw);
	grspw2_set_time_rx(&spw_cfg.spw);
	grspw2_tick_out_interrupt_enable(&spw_cfg.spw);

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

	application_load( (void*) 0x60200000, "mytest", KTHREAD_CPU_AFFINITY_NONE);

	{
		struct timespec t = get_uptime();
		printk("entering main idle loop at uptime %u s, %u ms\n", t.tv_sec, t.tv_nsec / 1000000);
	}
	while(1)
		cpu_relax();


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



