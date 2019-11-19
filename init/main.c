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



/** XXX dummy **/
extern int cpu1_ready;


volatile int xp;
int task1(void *p)
{
	while (1) {
		xp++;
	}
}

volatile int xd;
int task2(void *p)
{
	while (1)
		xd++;
}

int task(void *p)
{
	char buf1[64];
	char buf2[64];
	char buf3[64];

	struct sysobj *sys_irq = NULL;


	sys_irq = sysset_find_obj(sys_set, "/sys/irl/primary");

	if (!sys_irq) {
		printk("Error locating sysctl entry\n");
		return -1;
	}

	while (1) {

		sysobj_show_attr(sys_irq, "irl", buf1);
		sysobj_show_attr(sys_irq, "8", buf2);
		sysobj_show_attr(sys_irq, "9", buf3);

		printk("IRQ total: %s timer1: %s timer2: %s, %d %d\n",
		       buf1, buf2, buf3, ioread32be(&xp), xd);

		sched_yield();
	}

	return 0;
}



/** XXX dummy **/
extern int cpu_ready[CONFIG_SMP_CPUS_MAX];
/**
 * @brief kernel main functionputchar( *((char *) data) );
 */
int kernel_main(void)
{
	int i;
	struct task_struct *t;
	struct sched_attr attr;

#if 0
	void *addr;
	struct elf_module m;
#endif

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


#ifdef CONFIG_EMBED_APPLICATION
	/* dummy demonstrator */
{
	void *addr;
	struct elf_module m;

	addr = module_read_embedded("CrApp1");

	pr_debug(MSG "test executable address is %p\n", addr);
	if (addr)
		module_load(&m, addr);

#if 0
	modules_list_loaded();
#endif
}
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


	t = kthread_create(task, NULL, KTHREAD_CPU_AFFINITY_NONE, "task");
	if (!IS_ERR(t)) {
		sched_get_attr(t, &attr);
		attr.policy = SCHED_EDF;
		attr.period       = ms_to_ktime(1000);
		attr.deadline_rel = ms_to_ktime(999);
		attr.wcet         = ms_to_ktime(300);
		sched_set_attr(t, &attr);
		if (kthread_wake_up(t) < 0)
			printk("---- %s NOT SCHEDUL-ABLE---\n", t->name);
	} else {
		printk("Got an error in kthread_create!");
	}
	printk("%s runs on CPU %d\n", t->name, t->on_cpu);

	t = kthread_create(task1, NULL, KTHREAD_CPU_AFFINITY_NONE, "task1");
	if (!IS_ERR(t)) {
		sched_get_attr(t, &attr);
		attr.policy = SCHED_EDF;
		attr.period       = us_to_ktime(300);
		attr.deadline_rel = us_to_ktime(200);
		attr.wcet         = us_to_ktime(100);
		sched_set_attr(t, &attr);
		if (kthread_wake_up(t) < 0)
			printk("---- %s NOT SCHEDUL-ABLE---\n", t->name);
	} else {
		printk("Got an error in kthread_create!");
	}
	printk("%s runs on CPU %d\n", t->name, t->on_cpu);


	t = kthread_create(task2, NULL, KTHREAD_CPU_AFFINITY_NONE, "task2");
	if (!IS_ERR(t)) {
		sched_get_attr(t, &attr);
		attr.policy = SCHED_EDF;
		attr.period       = us_to_ktime(300);
		attr.deadline_rel = us_to_ktime(200);
		attr.wcet         = us_to_ktime(100);
		sched_set_attr(t, &attr);
		if (kthread_wake_up(t) < 0)
			printk("---- %s NOT SCHEDUL-ABLE---\n", t->name);
	} else {
		printk("Got an error in kthread_create!");
	}
	printk("%s runs on CPU %d\n", t->name, t->on_cpu);


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



