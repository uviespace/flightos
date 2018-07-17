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

#if 1
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
	//	schedule();
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
	/* registered initcalls are executed by the libgloss boot code at this
	 * time no need to do anything here at the moment.
	 */

	setup_arch();

	/* free_bootmem() */
	/* run_init_process() */

	return 0;
}
arch_initcall(kernel_init);


#include <kernel/clockevent.h>
void clk_event_handler(struct clock_event_device *ce)
{

	struct timespec expires;
	struct timespec now;

//	printk("DIIIING-DOOONG\n");
	get_ktime(&now);

	expires = now;

	expires.tv_sec += 1;

	clockevents_program_event(&clk_event_handler, expires, now, CLOCK_EVT_MODE_ONESHOT);
}


/**
 * @brief kernel main function
 */
#define MAX_TASKS 800
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



	{
	struct timespec expires;
	struct timespec now;
	get_ktime(&now);

	expires = now;
	expires.tv_nsec += 18000;

	BUG_ON(clockevents_program_event(NULL, expires, now, CLOCK_EVT_MODE_PERIODIC));

	}


	kernel = kthread_init_main();




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
		get_uptime(&ts);
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
		get_uptime(&ts);
		printk("total %d after %d s %d ns (%g)\n", tcnt, ts.tv_sec, ts.tv_nsec, (double) ts.tv_sec + (double) ts.tv_nsec / 1e9);
		BUG_ON(tcnt > MAX_TASKS);

		for (i = 0; i < tcnt; i++)
			kthread_wake_up(tasks[i]);

		arch_local_irq_disable();
		get_uptime(&ts);
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



