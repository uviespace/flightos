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
#include <modules-image.h>

#include <asm/processor.h>

/* for our demo */
#include "xentium_demo.h"


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

static volatile int *console = (int *)0x80000100;

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
struct task_struct *tsk1;
struct task_struct *tsk2;

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

		if (b > 3 * (int)c)
			break;
	//	schedule();
		//twiddle();
		//cpu_relax();
	}
	return 0;
}

int thread1(void *data)
{
	int b = 0;

	while(1) {
		//printk(".");
		int i;
		for (i = 0; i < 20; i++)
			putchar('.');
		putchar('\n');

		if (b++ > 20)
			break;

		schedule();
		//twiddle();
		cpu_relax();
	}
	return 0;
}

int thread2(void *data)
{
	int b = 0;

	while (1) {
		int i;
		for (i = 0; i < 20; i++) {
			putchar('o');
			b++;
		}

		putchar('\n');
		if (b > 200)
			break;
		schedule();

	}
		//schedule();
		//cpu_relax();
		printk("Actually, I left...\n");
		return 0xdeadbeef;
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



/**
 * @brief kernel main function
 */

int kernel_main(void)
{
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


#define GR712_IRL1_GPTIMER_2    10

#define LEON3_TIMER_EN 0x00000001       /* enable counting */
#define LEON3_TIMER_RL 0x00000002       /* reload at 0     */
#define LEON3_TIMER_LD 0x00000004       /* load counter    */
#define LEON3_TIMER_IE 0x00000008       /* irq enable      */
	{
	struct gptimer_unit *mtu = (struct gptimer_unit *) 0x80000300;

	printk("%s() entered\n", __func__);

	irq_request(8,  ISR_PRIORITY_NOW, dummy, NULL);

	mtu->scaler_reload = 5;
	/* abs min: 270 / (5+1) (sched printing) */
	/* abs min: 800 / (5+1) (threads printing) */
	mtu->timer[0].reload = 2000 / (mtu->scaler_reload + 1);
	mtu->timer[0].value = mtu->timer[0].reload;
	mtu->timer[0].ctrl = LEON3_TIMER_LD | LEON3_TIMER_EN
		| LEON3_TIMER_RL | LEON3_TIMER_IE;
	}


	kernel = kthread_init_main();

	tsk1 = kthread_create(thread1, NULL, KTHREAD_CPU_AFFINITY_NONE, "Thread1");
	tsk2 = kthread_create(thread2, NULL, KTHREAD_CPU_AFFINITY_NONE, "Thread2");
	//kthread_wake_up(tsk2);
	//	kthread_wake_up(tsk2);
	//
	{
	static char zzz[] = {':', '/', '\\', '~', '|'};
	int i;

	for (i = 0; i < ARRAY_SIZE(zzz); i++) 
		kthread_create(threadx, &zzz[i], KTHREAD_CPU_AFFINITY_NONE, "Thread2");
	}

	while(1) {
		//printk("-");
#if 0
		int i;
		for (i = 0; i < 20; i++)
			putchar('-');
		putchar('\n');
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



