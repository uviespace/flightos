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
#include <kernel/user.h>
#include <kernel/module.h>
#include <kernel/application.h>
#include <kernel/ksym.h>
#include <kernel/printk.h>
#include <kernel/kernel.h>
#include <kernel/kthread.h>
#include <kernel/time.h>
#include <kernel/watchdog.h>
#include <kernel/err.h>
#include <kernel/sysctl.h>
#include <kernel/smp.h>
#include <modules-image.h>

#include <kernel/string.h>
#include <kernel/kmem.h>
#include <kernel/edac.h>
#include <kernel/memscrub.h>

#include <asm/processor.h>

/* for our demo */
#include "xentium_demo.h"
/* arch irq disable/enable */
#include <asm/irqflags.h>


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


/**
 * @brief kernel initialisation routines
 */

static int kernel_init(void)
{
	int i;


	setup_arch();

	/* free_bootmem() */
	/* run_init_process() */

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

	return 0;
}
arch_initcall(kernel_init);



#ifdef CONFIG_ARCH_CUSTOM_BOOT_CODE

extern usercall_t __usercall_start;
extern usercall_t __usercall_end;

static void do_userspace(void)
{
	usercall_t *p = &__usercall_start;

	while(p < &__usercall_end) {
		if (*p)
			(*p)();
		p++;
	}
}
#endif /* CONFIG_ARCH_CUSTOM_BOOT_CODE */


/**
 * @brief kernel main functionputchar( *((char *) data) );
 */
int kernel_main(void)
{

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
#if 0
	/* XXX the image is not necessary aligned properly, so we can't access
	 * it directly, until we have a MNA trap */
	addr = module_read_embedded("testmodule.ko");

	pr_debug(MSG "noc_dma module address is %p\n", addr);
	if (addr)
		module_load(&m, addr);
#endif

#endif /* CONFIG_EMBED_MODULES_IMAGE */

#ifdef CONFIG_KERNEL_PRINTK
	{
		struct timespec t = get_uptime();
		printk("OS boot complete at uptime %u s, %u ms\n", t.tv_sec, t.tv_nsec / 1000000);
	}
#endif /* CONFIG_KERNEL_PRINTK */

#ifdef CONFIG_ARCH_CUSTOM_BOOT_CODE
	do_userspace();
#endif /* CONFIG_ARCH_CUSTOM_BOOT_CODE */

#ifdef CONFIG_KERNEL_PRINTK
	{
		struct timespec t = get_uptime();
		printk("entering main idle loop at uptime %u s, %u ms\n", t.tv_sec, t.tv_nsec / 1000000);
	}
#endif /* CONFIG_KERNEL_PRINTK */

	/* main loop of boot cpu */
	main_kernel_loop();

	return 0;	/* never reached */
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



