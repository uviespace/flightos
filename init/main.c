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

int task0(void *data)
{
	while (1) {

		xd++;

	//	printk("t1 %d %llu\n", leon3_cpuid(), ktime_to_us(ktime_get()));
	//	sched_yield();
	}
}

int task1(void *data)
{
	while (1) {

		xa++;

	//	printk("t1 %d %llu\n", leon3_cpuid(), ktime_to_us(ktime_get()));
	//	sched_yield();
	}
}


int task2(void *data)
{
	while (1) {
		//printk("x %llu\n", ktime_get());
		//printk("_");
		xb++;
	//	printk("t2 %d %llu\n", leon3_cpuid(), ktime_to_us(ktime_get()));
	//	sched_yield();
	//	printk("-");
	//	sched_yield();
	}
}
int task3(void *data)
{
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
	//	printk(".");
//		printk("t3 %d %llu\n", leon3_cpuid(), ktime_to_us(ktime_get()));
		xc++;

	//	sched_yield();
	}
}
#include <kernel/sysctl.h>
extern ktime sched_last_time;
	void sched_print_edf_list_internal(struct task_queue *tq, int cpu, ktime now);
extern uint32_t sched_ev;


extern struct scheduler sched_edf;
int task_rr(void *data)
{
	int last = 0;
	int curr = 0;
	char buf1[64];

	uint32_t last_call = 0;
	int a, b, c, d;

	ktime sched_time;
	struct sysobj *sys_irq = NULL;



	bzero(buf1, 64);
	sys_irq = sysset_find_obj(sys_set, "/sys/irl/primary");


	while (1) {

#if 0
		if (sys_irq)
			sysobj_show_attr(sys_irq, "irl", buf1);

		a = xa;
		b = xb;
		c = xc;
		d = xd;
		sched_time = sched_last_time;
		curr = atoi(buf1)/2;
		printk("%u %u %u %u %llu ", a, b, c, d, ktime_get());
//		printk("sched %llu us ", ktime_to_us(sched_last_time));
		printk("%llu per call ", sched_last_time /sched_ev);
//		printk("calls %d ", sched_ev - last_call);
		printk("cpu %d", leon3_cpuid());

		printk("\n");

		last = curr;
		last_call = sched_ev;
#endif

		sched_print_edf_list_internal(&sched_edf.tq[0], 0, ktime_get());
		sched_print_edf_list_internal(&sched_edf.tq[1], 1, ktime_get());




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
	kthread_init_main();
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



	cpu1_ready = 2;
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


#if 0

	t = kthread_create(task0, NULL, KTHREAD_CPU_AFFINITY_NONE, "task0");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = ms_to_ktime(100);
	attr.deadline_rel = ms_to_ktime(90);
	attr.wcet         = ms_to_ktime(39);
	sched_set_attr(t, &attr);
	kthread_wake_up(t);
#endif



#if 1
	t = kthread_create(task1, NULL, KTHREAD_CPU_AFFINITY_NONE, "task1");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = ms_to_ktime(50);
	attr.deadline_rel = ms_to_ktime(40);
	attr.wcet         = ms_to_ktime(33);
	sched_set_attr(t, &attr);
	kthread_wake_up(t);
#endif

#if 0
	t = kthread_create(task2, NULL, KTHREAD_CPU_AFFINITY_NONE, "task2");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = ms_to_ktime(40);
	attr.deadline_rel = ms_to_ktime(22);
	attr.wcet         = ms_to_ktime(17);
	sched_set_attr(t, &attr);
	kthread_wake_up(t);
#endif

#if 1
	t = kthread_create(task3, NULL, KTHREAD_CPU_AFFINITY_NONE, "task3");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_EDF;
	attr.period       = ms_to_ktime(80);
	attr.deadline_rel = ms_to_ktime(70);
	attr.wcet         = ms_to_ktime(13);
	sched_set_attr(t, &attr);
	kthread_wake_up(t);
#endif


#if 0
	t = kthread_create(task_rr, NULL, KTHREAD_CPU_AFFINITY_NONE, "task_rr");
	sched_get_attr(t, &attr);
	attr.policy = SCHED_RR;
	attr.priority = 1;
	sched_set_attr(t, &attr);
	kthread_wake_up(t);
#endif

#endif




}
#endif

	while (ioread32be(&cpu1_ready) != 0x3);
      iowrite32be(0x4, &cpu1_ready);
	while(1) {
//		printk("o");
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
//		printk("cpu1\n");
		cpu_relax();
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



