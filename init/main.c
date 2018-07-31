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


#include <kernel/irq.h>

irqreturn_t dummy(unsigned int irq, void *userdata)
{
	// printk("IRQ!\n");
	//schedule();
	return 0;
}


/**
 * @brief do something useless
 */
__attribute__((unused))
static void twiddle(void)
{
	static int i;
	const char cursor[] = {'/', '-', '\\', '|'};

	printk("%c\b\b ", cursor[i]);

	i = (i + 1) % ARRAY_SIZE(cursor);
}



#define TREADY 4

#if 0
static volatile int *console = (int *)0x80100100;
#else
static volatile int *console = (int *)0x80000100;
#endif

static int putchar(int c)
{
	while (!(console[1] & TREADY));

	console[0] = 0x0ff & c;

	if (c == '\n') {
		while (!(console[1] & TREADY));
		console[0] = (int) '\r';
	}

	return c;
}



int edf1(void *data)
{
	struct timespec ts;
	static struct timespec t0;
	int i;

	while (1) {
		ts = get_ktime();
#if 0
		printk("\tedf1: %g s; delta: %g s (%g Hz)\n",
		       (double) ts.tv_sec + (double) ts.tv_nsec / 1e9,
		       difftime(ts, t0), 1.0/difftime(ts, t0));
#endif
		for (i = 0; i < 100000; i++)
			putchar('.');


		t0 = ts;
		sched_yield();
	}
}

int edf2(void *data)
{
	while (1)
		putchar( *((char *) data) );
}


int edf3(void *data)
{
	while (1) {
		putchar( *((char *) data) );
		putchar( *((char *) data) );
		sched_yield();
	}
}

int edf4(void *data)
{
	while (1) {
		//sched_print_edf_list();
		putchar('-');
		sched_yield();
	}
}


int add0r(void *data)
{
	volatile int *add = (int *) data;

	while (1) 
		add[0]++;
}

int print0r(void *data)
{
	int i;
	int *s = (int *) data;
	static int addx[30];

	while (1) {
		for (i = 0; i < 30; i++)
			addx[i] = s[i];

		printk("delta: ");
		for (i = 0; i < 30; i++)
			 printk("%d ", s[i]- addx[i]);
		printk("\nabs:   ");
		for (i = 0; i < 30; i++)
			 printk("%d ", s[i]);
		printk("\n\n");
		sched_yield();
	}
}


extern struct task_struct *kernel;

int threadx(void *data)
{

	char c = (char) (* (char *)data);
	int b = 0;

	while(1) {
		//printk(".");
		int i;
		for (i = 0; i < (int) c; i++) {
			putchar(c);
			b++;
		}
		putchar('\n');
#if 1
		if (b > (int) c * (int)c)
			break;
#endif
	//	schedule();putchar( *((char *) data) );
		//twiddle();
		//cpu_relax();
	}
	return 0;
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
#define MAX_TASKS 0
#include <kernel/clockevent.h>
int kernel_main(void)
{
	struct task_struct *tasks[MAX_TASKS];
	int tcnt = 0;
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



	kernel = kthread_init_main();


#if 0
	{
	struct task_struct *t1;
	t1 = kthread_create(edf1, NULL, KTHREAD_CPU_AFFINITY_NONE, "Thread2");
	kthread_set_sched_edf(t1, 2e5,  1e5);
	kthread_wake_up(t1);
	}
#endif
#if 0
	{
	struct task_struct *t2;
	struct task_struct *t3;

	t2 = kthread_create(edf3, "\n", KTHREAD_CPU_AFFINITY_NONE, "EDF%d", 1);
	kthread_set_sched_edf(t2, 1 * USEC_PER_SEC,  40 * USEC_PER_MSEC, 60 * USEC_PER_MSEC + 1);
	kthread_wake_up(t2);


	t2 = kthread_create(edf3, "?", KTHREAD_CPU_AFFINITY_NONE, "EDF%d", 1);
	kthread_set_sched_edf(t2, 1 * USEC_PER_SEC,  40 * USEC_PER_MSEC, 60 * USEC_PER_MSEC + 1);
	kthread_wake_up(t2);

	t3 = kthread_create(edf4, NULL, KTHREAD_CPU_AFFINITY_NONE, "EDF_other");
	kthread_set_sched_edf(t3, 200 * USEC_PER_MSEC,  10 * USEC_PER_MSEC, 10 * USEC_PER_MSEC + 1);
	kthread_wake_up(t3);

	t3 = kthread_create(edf2, "x", KTHREAD_CPU_AFFINITY_NONE, "EDF_otherx");
	kthread_set_sched_edf(t3, 300 * USEC_PER_MSEC,  5 * USEC_PER_MSEC, 5 * USEC_PER_MSEC + 1);
	kthread_wake_up(t3);

	t3 = kthread_create(edf2, ":", KTHREAD_CPU_AFFINITY_NONE, "EDF_otherx");
	kthread_set_sched_edf(t3, 50 * USEC_PER_MSEC,  5 * USEC_PER_MSEC, 5 * USEC_PER_MSEC + 1);
	kthread_wake_up(t3);

	t3 = kthread_create(edf2, "o", KTHREAD_CPU_AFFINITY_NONE, "EDF_otherx");
	kthread_set_sched_edf(t3, 50 * USEC_PER_MSEC,  5 * USEC_PER_MSEC, 5 * USEC_PER_MSEC + 1);
	kthread_wake_up(t3);

	t3 = kthread_create(edf2, "/", KTHREAD_CPU_AFFINITY_NONE, "EDF_otherx");
	kthread_set_sched_edf(t3, 30 * USEC_PER_MSEC,  3 * USEC_PER_MSEC, 3 * USEC_PER_MSEC + 1);
	kthread_wake_up(t3);

	t3 = kthread_create(edf2, "\\", KTHREAD_CPU_AFFINITY_NONE, "EDF_otherx");
	kthread_set_sched_edf(t3, 6 * USEC_PER_MSEC,  2 * USEC_PER_MSEC, 2 * USEC_PER_MSEC + 1);
	kthread_wake_up(t3);

	}
#endif


#if 1
	{
		int i;
		struct task_struct *t;
		static int add[30];

		t = kthread_create(print0r, add, KTHREAD_CPU_AFFINITY_NONE, "print");
		kthread_set_sched_edf(t, 1e6,  300 * USEC_PER_MSEC, 300 * USEC_PER_MSEC + 1);
		kthread_wake_up(t);
#if 1

		for (i = 0; i < 30; i++) {
			t = kthread_create(add0r, &add[i], KTHREAD_CPU_AFFINITY_NONE, "EDF%d", i);
			kthread_set_sched_edf(t, 45 * USEC_PER_MSEC,  1 * USEC_PER_MSEC, 1 * USEC_PER_MSEC + 1);
			kthread_wake_up(t);
		}
#endif
	}
#endif








	while(1) {
	//	putchar('o');
#if 0
		static ktime t0;
		ktime ts;

		ts = ktime_get();

		printk("now: %llu %llu delta %lld\n\n", ts, t0, ts-t0);
		t0 = ts;
#endif
		cpu_relax();
	}



	while(1) {
		struct timespec ts;
		static struct timespec t0;

		ts = get_ktime();
		//printk("now: %g s; delta: %g ns (%g Hz)\n", (double) ts.tv_sec + (double) ts.tv_nsec / 1e9, difftime(ts, t0), 1.0/difftime(ts, t0) );
		t0 = ts;
		cpu_relax();
	}

	{
	static char zzz[] = {':', '/', '\\', '~', '|'};
	int i;

	for (i = 0; i < ARRAY_SIZE(zzz); i++)
		kthread_create(threadx, &zzz[i], KTHREAD_CPU_AFFINITY_NONE, "Thread2");
	}

	{
		static char zzz[] = {':', '/', '\\', '~', '|'};
		static int z;
		char *buf = NULL;
		int i;
		struct timespec ts;
		ts = get_uptime();
		printk("creating tasks at %d s %d ns (%g)\n", ts.tv_sec, ts.tv_nsec, (double) ts.tv_sec + (double) ts.tv_nsec / 1e9);



		for (i = 0; i < MAX_TASKS; i++) {
		//	buf = kmalloc(30);
		//	BUG_ON(!buf);

		//	sprintf(buf, "Thread %d", z);
			z++;

			tasks[tcnt++] = kthread_create(threadx, &zzz[i], KTHREAD_CPU_AFFINITY_NONE, buf);
		//	kfree(buf);
		}

	}


	{
		int i;

		struct timespec ts;
		ts = get_uptime();
		printk("total %d after %d s %d ns (%g)\n", tcnt, ts.tv_sec, ts.tv_nsec, (double) ts.tv_sec + (double) ts.tv_nsec / 1e9);
		BUG_ON(tcnt > MAX_TASKS);

		for (i = 0; i < tcnt; i++)
			kthread_wake_up(tasks[i]);

		arch_local_irq_disable();
		ts = get_uptime();
		printk("all awake after %d s %d ns (%g)\n", ts.tv_sec, ts.tv_nsec, (double) ts.tv_sec + (double) ts.tv_nsec / 1e9);
		arch_local_irq_enable();
	}


	while(1) {
		twiddle();
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



