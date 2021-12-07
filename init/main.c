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
	struct elf_module m __attribute__((unused));



#ifdef CONFIG_EMBED_MODULES_IMAGE
	printk(MSG "Loading module image\n");

	/* load the embedded AR image */
	module_image_load_embedded();

	/* look up a kernel symbol */
	printk(MSG "Looked up symbol %s at %p\n",
	       "printk", lookup_symbol("printk"));

	/* look up a file in the embedded image */
	printk(MSG "Looked up file %s at %p\n", "noc_dma.ko",
	       module_lookup_embedded("noc_dma.ko"));

#if 0
	/* If you have kernel modules embedded in your modules.images, this
	 * is how you can access them:
	 * lookup the module containing <symbol>
	 */
	addr = module_lookup_symbol_embedded("somefunction");

	/* XXX the image is not necessary aligned properly, so we can't access
	 * it directly, until we have a MNA trap */
	addr = module_read_embedded("noc_dma.ko");

	pr_debug(MSG "noc_dma module address is %p\n", addr);
	if (addr)
		module_load(&m, addr);

	modules_list_loaded();
#endif
#endif


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

		//printk("waiting for cpu %d, flag at %d\n", i, cpu_ready[i]);
		cpu_ready[i] = 2;
		while (ioread32be(&cpu_ready[i]) != 0x3);
		iowrite32be(0x4, &cpu_ready[i]);

	}

	printk(MSG "Boot complete\n");

#ifdef CONFIG_EMBED_APPLICATION
#if 0
	/* dummy demonstrator */
	addr = module_read_embedded("executable_demo");
#else
	/* CrIa */
	addr = module_read_embedded("myapp");
#endif

	addr = (void *) 0x60000000;
	pr_info(MSG "test executable address is %p\n", addr);
	if (addr)
		module_load(&m, addr);

#if 0
	modules_list_loaded();
#endif
#endif	/* CONFIG_EMBED_APPLICATION */

#if 0
	copybench_start();
	oneshotedf_start();
#endif
	printk("entering main idle loop\n");
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



