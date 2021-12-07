/**
 * @file arch/sparc/kernel/setup.c
 *
 * @ingroup sparc
 * @defgroup sparc SPARC
 * @brief the SPARC architecture-specific implementation
 *
 */

#include <string.h> /* memset() */

#include <init.h>
#include <mm.h>
#include <asm/irq.h>
#include <asm/time.h>
#include <asm/clockevent.h>
#include <kernel/clockevent.h>
#include <compiler.h>

#include <page.h>
#include <stack.h>
#include <kernel/kmem.h>

#include <kernel/sched.h>

#include <kernel/smp.h>
#include <asm/irqflags.h>

void *_kernel_stack_top;
void *_kernel_stack_bottom;


/**
 * @brief reserve a stack area for the kernel
 *
 * @warn Since we allocate the kernel stack using kmalloc instead of placing it
 *	 in a custom area, there is a real change of violating the bottom
 *	 boundary, so make sure you allocate enough pages to fit your needs.
 *	 Note that since kmalloc() works via kernel_sbrk() and moving the system
 *	 break does not actually reserve pages until they are accessed, you
 *	 have to initialise the stack area, i.e. actually reserve the pages,
 *	 unless you have a custom interrupt/trap stack. Otherwise the kernel
 *	 cannot perform stack access to an unmapped page, because that would
 *	 require a mapped page...
 *
 * XXX this needs to be addressed at some point, but probably only after we
 *     have a kernel bootstrap implemented and we may define custom reserved
 *     areas more freely.
 */

#warning "Using fixed-size kernel stack"
static void reserve_kernel_stack(void)
{
	const size_t k_stack_sz = KERNEL_STACK_PAGES * PAGE_SIZE;


	/* the bottom of the stack */
	_kernel_stack_bottom = kcalloc(k_stack_sz + STACK_ALIGN, sizeof(char));
	BUG_ON(!_kernel_stack_bottom);

	/* the (aligned) top of the stack */
	_kernel_stack_top = (void *) (char *) _kernel_stack_bottom + k_stack_sz;
	_kernel_stack_top = ALIGN_PTR(_kernel_stack_top, STACK_ALIGN);
}


/**
 * @brief configure available memory banks
 *
 * TODO the memory layout should either be presented in a separate
 *	board configuration file or, preferably, be derived from an AMBA
 *	bus scan.
 */

static void mem_init(void)
{
#ifdef CONFIG_MPPB
	sp_banks[0].base_addr = 0x40000000;
	sp_banks[0].num_bytes = 0x10000000;
#endif

#ifdef CONFIG_LEON4
	sp_banks[0].base_addr = 0x00000000;
	sp_banks[0].num_bytes = 0x10000000;
#else /* e.g. GR712 eval */
	sp_banks[0].base_addr = 0x40000000;
	sp_banks[0].num_bytes = 0x00800000;
#endif

#if (SPARC_PHYS_BANKS > 0)
	sp_banks[1].base_addr = 0x60100000;
	sp_banks[1].num_bytes = 0x04000000-0x100000;
#warning "SPARC PHYS BANK 2 currently reconfigured for start at 0x60100000"
#else
#warning "Configuration error: SPARC_PHYS_BANKS size insufficient."
#endif

}


int cpu_ready[CONFIG_SMP_CPUS_MAX];


#include <asm/io.h>
/* wake a cpu by writing to the multiprocessor status register */
void cpu_wake(uint32_t cpu_id)
{
#ifdef CONFIG_LEON3
	iowrite32be(1 << cpu_id, (uint32_t *) 0x80000210);
#endif
#ifdef CONFIG_LEON4
	iowrite32be(1 << cpu_id, (uint32_t *) 0xFF904010);
#endif
}

/** XXX crappy */
static void boot_cpus(void)
{
	int i;


	for (i = 1; i < CONFIG_SMP_CPUS_MAX; i++) {

		pr_info("booting cpu %d\n", i);
		cpu_wake(i);

		while (!ioread32be(&cpu_ready[i]));
		pr_info("cpu %d booted\n", i);

	}
}


#include <asm/processor.h>
#include <kernel/kthread.h>
extern struct task_struct *kernel[];
void smp_cpu_entry(void)
{

	reserve_kernel_stack();
	BUG_ON(stack_migrate(NULL, _kernel_stack_top));

	arch_local_irq_enable();

	pr_info("hi i'm cpu %d\n", leon3_cpuid());

	BUG_ON(!leon3_cpuid());
	/* signal ready */
	iowrite32be(0x1, &cpu_ready[leon3_cpuid()]);

	while (ioread32be(&cpu_ready[leon3_cpuid()]) != 0x2);

	BUG_ON(clockevents_offer_device()); /* XXX CLOCK */
	kthread_init_main();

	iowrite32be(0x3, &cpu_ready[leon3_cpuid()]);
	while (ioread32be(&cpu_ready[leon3_cpuid()]) != 0x4);

	sched_enable();

	while(1)
		cpu_relax();
}





/**
 * @brief architecture setup entry point
 */

void setup_arch(void)
{
	mem_init();

	paging_init();

	BUG_ON(!kmem_init());

	reserve_kernel_stack();

	BUG_ON(stack_migrate(NULL, _kernel_stack_top));

	leon_irq_init();

	sparc_uptime_init();

	sparc_clockevent_init();

	smp_init();

	boot_cpus();

	sched_enable();
}
