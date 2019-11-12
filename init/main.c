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

volatile int inc;
volatile unsigned int xa, xb, xc, xd;

int task2(void *data);

int task0(void *data)
{

	while (1) {

		xd++;

		if (xd % 1000000000 == 0)
			sched_yield();


	}
}

int task1(void *data)
{
	xa = 0;
	while (1) {

		xa++;

	}
}
#define QUIT_AT		100000

int task2(void *data)
{

	iowrite32be(0xdeadbeef, &xb);
	return 0;

	while (1) {
		xb++;


		if (xb > QUIT_AT) {
			printk("EXITING\n");
			xb = 0xdeadbeef;
			return 0;
		}
	}
}
extern int threadcnt;
int task_rr(void *data);
int task3(void *data)
{
	while (1) {
		xc++;
		task_rr(NULL);
	}
}

int xf;
int task4(void *data)
{
	while (1) {
		xf++;
	}
}


int task_restart(void *data)
{
	struct task_struct *t = NULL;
	struct sched_attr attr;

	xb = 0xdeadbeef;
	while(1) {
		/* "fake" single shot reset */


#if 1
		if (ioread32be(&xb) == 0xdeadbeef)
		{
			xb = 0;

			t = kthread_create(task2, NULL, KTHREAD_CPU_AFFINITY_NONE, "task7");

		//	printk("now at %p %d\n", t, threadcnt);
			sched_get_attr(t, &attr);
			attr.policy = SCHED_EDF;

			attr.period       = us_to_ktime(0);
			attr.deadline_rel = us_to_ktime(100);
			attr.wcet         = us_to_ktime(30);

			sched_set_attr(t, &attr);
			barrier();
			BUG_ON (kthread_wake_up(t) < 0);
			barrier();


		}
			sched_yield();
#endif
	}
}


#include <kernel/sysctl.h>
extern ktime sched_last_time;
	void sched_print_edf_list_internal(struct task_queue *tq, int cpu, ktime now);
extern uint32_t sched_ev;


extern struct scheduler sched_edf;
int task_rr(void *data)
{
	char buf1[64];
	char buf2[64];
	char buf3[64];


	struct sysobj *sys_irq = NULL;



	bzero(buf1, 64);
	sys_irq = sysset_find_obj(sys_set, "/sys/irl/primary");


	while (1) {

		if (sys_irq) {
			sysobj_show_attr(sys_irq, "irl", buf1);
			sysobj_show_attr(sys_irq, "8", buf2);
			sysobj_show_attr(sys_irq, "9", buf3);
			printk("IRQs: %s timer1 %s timer2 %s threads created: %d\n", buf1, buf2, buf3, threadcnt);
		}

	//	sched_print_edf_list_internal(&sched_edf.tq[0], 0, ktime_get());
	//	sched_print_edf_list_internal(&sched_edf.tq[1], 1, ktime_get());




		sched_yield();
	}
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



/** XXX dummy **/
extern int cpu1_ready;
/**
 * @brief kernel main functionputchar( *((char *) data) );
 */
#define MAX_TASKS 0
#include <kernel/clockevent.h>
#include <kernel/tick.h>
int kernel_main(void)
{
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
	tick_set_next_ns(1000000);

	/* wait for cpus */
	cpu1_ready = 2;

	while (ioread32be(&cpu1_ready) != 0x3);
	iowrite32be(0x4, &cpu1_ready);

	printk(MSG "Boot complete\n");




#if 0
	t = kthread_create(task2, NULL, KTHREAD_CPU_AFFINITY_NONE, "print1");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = ms_to_ktime(1000);
	attr.deadline_rel = ms_to_ktime(900);
	attr.wcet         = ms_to_ktime(200);
	sched_set_attr(t, &attr);
	kthread_wake_up(t);
#endif

#if 1

	//t = kthread_create(task0, NULL, KTHREAD_CPU_AFFINITY_NONE, "task0");
	t = kthread_create(task_restart, NULL, KTHREAD_CPU_AFFINITY_NONE, "task_restart");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = ms_to_ktime(10);
	attr.deadline_rel = ms_to_ktime(9);
	attr.wcet         = ms_to_ktime(5);
	sched_set_attr(t, &attr);
	if (kthread_wake_up(t) < 0)
		printk("---- %s NOT SCHEDUL-ABLE---\n", t->name);
#endif



#if 0
	t = kthread_create(task1, NULL, KTHREAD_CPU_AFFINITY_NONE, "task1");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = us_to_ktime(50000);
	attr.deadline_rel = us_to_ktime(40000);
	attr.wcet         = us_to_ktime(33000);
	sched_set_attr(t, &attr);
	if (kthread_wake_up(t) < 0)
		printk("---- %s NOT SCHEDUL-ABLE---\n", t->name);
#endif

#if 0
	t = kthread_create(task2, NULL, KTHREAD_CPU_AFFINITY_NONE, "task2");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = us_to_ktime(200);
	attr.deadline_rel = us_to_ktime(110);
	attr.wcet         = us_to_ktime(95);
	sched_set_attr(t, &attr);
	if (kthread_wake_up(t) < 0) {
		printk("---- %s NOT SCHEDUL-ABLE---\n", t->name);
		BUG();
	}
#endif

#if 1
	t = kthread_create(task3, NULL, KTHREAD_CPU_AFFINITY_NONE, "task3");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = ms_to_ktime(1000);
	attr.deadline_rel = ms_to_ktime(999);
	attr.wcet         = ms_to_ktime(300);
	sched_set_attr(t, &attr);
	if (kthread_wake_up(t) < 0)
		printk("---- %s NOT SCHEDUL-ABLE---\n", t->name);
#endif


#if 0
	t = kthread_create(task_rr, NULL, KTHREAD_CPU_AFFINITY_NONE, "task_rr");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_RR;
	attr.priority = 1;
	sched_set_attr(t, &attr);
	kthread_wake_up(t);
#endif


#if 0
	t = kthread_create(task_restart, NULL, KTHREAD_CPU_AFFINITY_NONE, "task_restart");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_RR;
	attr.priority = 1;
	sched_set_attr(t, &attr);
	kthread_wake_up(t);
#endif
//	xb = 0xdeadbeef;
	while(1) {
		/* "fake" single shot reset */

	//	task_rr(NULL);
#if 0
		if (xb == 0xdeadbeef)
		{
			xb = 0;
			t = kthread_create(task2, NULL, KTHREAD_CPU_AFFINITY_NONE, "task2");
			sched_get_attr(t, &attr);
			attr.policy = SCHED_EDF;

			attr.period       = us_to_ktime(0);
			attr.deadline_rel = us_to_ktime(100);
			attr.wcet         = us_to_ktime(60);

			sched_set_attr(t, &attr);
			BUG_ON (kthread_wake_up(t) < 0);

		}
#endif

		cpu_relax();
	}

	while (1)
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



