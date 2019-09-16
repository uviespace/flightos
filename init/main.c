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
volatile int xa, xb, xc;
int task1(void *data)
{
	while (1) {

		xa++;

		//printk("#");
//		sched_yield();
	}
}


int task2(void *data)
{
	while (1) {
		//printk("x %llu\n", ktime_get());
		//printk("_");
		xb++;
	//	sched_yield();
//		printk("-");
	//	sched_yield();
	}
}
static int cnt;
static	ktime buf[1024];
int task3(void *data)
{
	ktime last = 0;

	while (1) {
#if 0
		ktime now;
		if (cnt < 1024) {
			now = ktime_get();
			buf[cnt] = ktime_delta(now, last);
			last = now;
			cnt++;
		}
	       //	else
		//	sched_yield();

#endif
		//printk("y %llu\n", ktime_get());
		//printk(".");
		xc++;

	//	sched_yield();
	}
}


int task0(void *data)
{
	int a, b, c;
	while (1) {
		a = xa;
		b = xb;
		c = xc;
		printk("%d %d %d\n", a, b, c);
//		sched_yield();
	}
}

extern struct task_struct *kernel;


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
#define MAX_TASKS 0
#include <kernel/clockevent.h>
int kernel_main(void)
{
	struct task_struct *t;

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
	printk(MSG "Boot complete, spinning idly.\n");



	/* elevate boot thread */
	kernel = kthread_init_main();
#if 0
	/*
	 *  T1: (P=50, D=20, R=10)
	 *
	 *  T2: (P= 4, D= 2, R= 1)
	 *  T5: (P=20, D=12, R= 5)
	 *
	 *  T6: (P=33, D=30, R= 4)
	 *  T7: (P=50, D=46, R= 6)
	 */

	t = kthread_create(task1, NULL, KTHREAD_CPU_AFFINITY_NONE, "T1");
	kthread_set_sched_edf(t, 50 * MSEC_PER_SEC,  10 * MSEC_PER_SEC, 20 * MSEC_PER_SEC);


	t = kthread_create(task1, NULL, KTHREAD_CPU_AFFINITY_NONE, "T2");
	kthread_set_sched_edf(t, 4 * MSEC_PER_SEC,  1 * MSEC_PER_SEC, 2 * MSEC_PER_SEC);

	t = kthread_create(task1, NULL, KTHREAD_CPU_AFFINITY_NONE, "T5");
	kthread_set_sched_edf(t, 20 * MSEC_PER_SEC, 5 * MSEC_PER_SEC, 12 * MSEC_PER_SEC);


	t = kthread_create(task1, NULL, KTHREAD_CPU_AFFINITY_NONE, "T6");
	kthread_set_sched_edf(t, 33 * MSEC_PER_SEC, 4 * MSEC_PER_SEC, 30 * MSEC_PER_SEC);
	t = kthread_create(task1, NULL, KTHREAD_CPU_AFFINITY_NONE, "T7");
	kthread_set_sched_edf(t, 50 * MSEC_PER_SEC, 6 * MSEC_PER_SEC, 46 * MSEC_PER_SEC);

#endif




#if 1
{
	struct sched_attr attr;
#if 0
	t = kthread_create(task1, NULL, KTHREAD_CPU_AFFINITY_NONE, "print");
	sched_get_attr(t, &attr);
	attr.priority = 4;
	sched_set_attr(t, &attr);
	kthread_wake_up(t);

	t = kthread_create(task2, NULL, KTHREAD_CPU_AFFINITY_NONE, "print1");
	sched_get_attr(t, &attr);
	attr.priority = 8;
	sched_set_attr(t, &attr);
	kthread_wake_up(t);
#endif
#if 1
	t = kthread_create(task2, NULL, KTHREAD_CPU_AFFINITY_NONE, "print1");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = us_to_ktime(1000);
	attr.deadline_rel = us_to_ktime(900);
	attr.wcet         = us_to_ktime(200);
	sched_set_attr(t, &attr);
	kthread_wake_up(t);

	t = kthread_create(task3, NULL, KTHREAD_CPU_AFFINITY_NONE, "print2");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = us_to_ktime(800);
	attr.deadline_rel = us_to_ktime(700);
	attr.wcet         = us_to_ktime(100);
	sched_set_attr(t, &attr);
	kthread_wake_up(t);

	t = kthread_create(task1, NULL, KTHREAD_CPU_AFFINITY_NONE, "print3");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = us_to_ktime(400);
	attr.deadline_rel = us_to_ktime(200);
	attr.wcet         = us_to_ktime(100);
	sched_set_attr(t, &attr);
	kthread_wake_up(t);

	t = kthread_create(task0, NULL, KTHREAD_CPU_AFFINITY_NONE, "res");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = ms_to_ktime(2000);
	attr.deadline_rel = ms_to_ktime(900);
	attr.wcet         = ms_to_ktime(100);
	sched_set_attr(t, &attr);
	kthread_wake_up(t);


#endif




}
#endif
	while(1) {
#if 0
		int val = inc;
		static ktime last;
		ktime now;
		now = ktime_get();
		static ktime delta;

		delta	= ktime_delta(last, now);
		last = now;
		printk("%d %lld\n", val, ktime_to_us(delta));
#endif
#if 0
	static int i;
	static ktime last;
		ktime now;
		now = ktime_get();
		if (i == 10) {
		printk("%lld\n", ktime_to_ms(ktime_delta(now, last)));
		last = now;
		i = 0;
		}
		i++;
#endif
	//	sched_yield();
#if 0
		if (cnt > 1023) {
			int i;
			for (i = 1; i < 1024; i++)
				printk("%lld\n", buf[i]);
		//	cnt = 0;
			break;
		}
#endif
//		printk("xxx %llu\n", ktime_get());

		//printk("%d\n", cnt);

	//	printk("o");
	//	printk("\n");

	//	sched_yield();
	//	cpu_relax();
	}

		//printk("%lld\n", buf[i]);

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



