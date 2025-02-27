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
#include <asm/edac.h>
#include <asm/time.h>
#include <asm/clockevent.h>
#include <asm/processor.h>

#include <kernel/clockevent.h>
#include <compiler.h>

#include <page.h>
#include <stack.h>
#include <kernel/kmem.h>

#include <kernel/time.h>
#include <kernel/kthread.h>
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
#if 0
#warning "Using fixed-size kernel stack"
#endif
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
#endif

#ifdef CONFIG_LEON3


#ifdef CONFIG_PROJECT_ARIEL
	/* XXX as above... */
	sp_banks[0].base_addr = 0x40000000;
	sp_banks[0].num_bytes = 0x00800000;	/* first 8 MiB */

#if (SPARC_PHYS_BANKS > 0)
	sp_banks[1].base_addr = 0x40800000;
	sp_banks[1].num_bytes = 0x00400000;	/* next 4 MiB */
#endif

#if (SPARC_PHYS_BANKS > 1)
	sp_banks[2].base_addr = 0x40C00000;
	sp_banks[2].num_bytes = 0x00200000;	/* next 2 MiB */
#endif

#if (SPARC_PHYS_BANKS > 2)
	sp_banks[3].base_addr = 0x40E00000;
	sp_banks[3].num_bytes = 0x00100000;	/* next 1 MiB */
#endif

#if (SPARC_PHYS_BANKS > 3)
	sp_banks[4].base_addr = 0x40F00000;
	sp_banks[4].num_bytes = 0x00080000;	/* next 512 kiB */
#endif
	/* ignore the remaing unused space, it is the location of the ASW */
#elif CONFIG_PROJECT_SMILE
	/* XXX need something like CONFIG_SOC_SMILE_SXI */
	/* XXX the base address is defined by the requirements
	 * (DBS RAM + exchange area).
	 * We also need some space at the end to run the ASW starting from
	 * addr 0x6FF00000, so we'll use several power-of-two partitions for now
	 * NOTE: this will work properly only when MMU support is enabled
	 */
	sp_banks[0].base_addr = 0x60040000;
	sp_banks[0].num_bytes = 0x08000000;	/* first 128 MiB */

#if (SPARC_PHYS_BANKS > 0)
	sp_banks[1].base_addr = 0x68040000;
	sp_banks[1].num_bytes = 0x04000000;	/* next 64 MiB */
#endif

#if (SPARC_PHYS_BANKS > 1)
	sp_banks[2].base_addr = 0x6C040000;
	sp_banks[2].num_bytes = 0x02000000;	/* next 32 MiB */
#endif

#if (SPARC_PHYS_BANKS > 2)
	sp_banks[3].base_addr = 0x6E040000;
	sp_banks[3].num_bytes = 0x01000000;	/* next 16 MiB */
#endif

#if (SPARC_PHYS_BANKS > 3)
	sp_banks[4].base_addr = 0x6F040000;
	sp_banks[4].num_bytes = 0x00800000;	/* next 8 MiB */
#endif

#if (SPARC_PHYS_BANKS > 4)
	sp_banks[5].base_addr = 0x6F840000;
	sp_banks[5].num_bytes = 0x00400000;	/* next 4 MiB */
#endif

#if (SPARC_PHYS_BANKS > 5)
	sp_banks[6].base_addr = 0x6FC40000;
	sp_banks[6].num_bytes = 0x00200000;	/* next 2 MiB */
#endif

#if (SPARC_PHYS_BANKS > 6)
	sp_banks[7].base_addr = 0x6FE40000;
	sp_banks[7].num_bytes = 0x00100000;	/* next 1 MiB */
#endif
	/* ignore the remaing unused space */

#else	/* e.g. GR712 eval */
	sp_banks[0].base_addr = 0x40000000;
	sp_banks[0].num_bytes = 0x00800000;

	/* use only first 64 MiB of SDRAM for our application demo */
	sp_banks[1].base_addr = 0x60000000;
	sp_banks[1].num_bytes = 0x04000000;
#endif /* CONFIG_PROJECT_SMILE */





#endif /* CONFIG_LEON3 */



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


/* XXX this is needed for SXI DPU; the boot sw cannot start
 * in SMP mode, so the second CPU is still at 0x0 initially
 *
 * we temporarily do this here to get ready for the EMC test
 *
 * we use the LEON3 debug support unit for this
 */
#include <leon3_dsu.h>
static void sxi_dpu_setup_cpu_entry(void)
{
	uint32_t tmp;


	dsu_set_noforce_debug_mode(1);
	dsu_set_cpu_break_on_iu_watchpoint(1);

	dsu_set_force_debug_on_watchpoint(1);

	/* set trap base register to be the same as on CPU0 and point
	 * %pc and %npc there
	 */
	tmp = dsu_get_reg_tbr(0) & ~0xfff;

	dsu_set_reg_tbr(1, tmp);

	dsu_set_reg_pc(1, tmp);
	dsu_set_reg_npc(1, tmp + 0x4);

	dsu_clear_iu_reg_file(1);
	/* default invalid mask */
	dsu_set_reg_wim(1, 0x2);

	/* set CWP to 7 */
	dsu_set_reg_psr(1, 0xf34010e1);

	dsu_clear_cpu_break_on_iu_watchpoint(1);
	/* resume cpu 1 */
	dsu_clear_force_debug_on_watchpoint(1);
}


/**
 * @brief update the cpu load
 */

static void cpu_load_update(void)
{
	int cpu;
	uint8_t load;

	ktime now;
	static ktime last[CONFIG_SMP_CPUS_MAX];


	cpu = smp_cpu_id();

	now = ktime_get();
	if (ktime_ms_delta(now, last[cpu]) > 1000) {
		load = (uint8_t) (100 - (kthread_get_total_runtime() * 100) / ktime_delta(now, last[cpu]));
		sched_set_cpu_load(cpu, load);
		kthread_clear_total_runtime();
		last[cpu] = now;
	}
}


/**
 * @brief per-cpu main kernel loop
 */

void main_kernel_loop(void)
{
	while(1) {
		cpu_load_update();
		cpu_relax();
	}
	BUG();	/* never reached */
}


/* XXX */
#include <asm/processor.h>
#include <kernel/kthread.h>
void srmmu_init_per_cpu(void);
extern struct task_struct *kernel[];
void smp_cpu_entry(void)
{

#ifdef CONFIG_MMU 
	srmmu_init_per_cpu();
#endif /* CONFIG_MMU */

	reserve_kernel_stack();
	BUG_ON(stack_migrate(NULL, _kernel_stack_top));

	arch_local_irq_enable();


#if 1
	/**
	 *
	 * XXX this was relevant for cheops, but the SXI DPU is basically
	 * set up identically, we add this for the EMC test just to make sure
	 *
	 * Upon startup, the FPU of the second core has random initial values,
	 * including the FSR. Here we set the CPU 1 FSR such as to disable
	 * all FPU traps.
	 *
	 * 0x0F800000 ... enable all FP exceptions (except NS)
	 * 0x08000000 ... operand error (NV)
	 * 0x06000000 ... rounding errors (OF + UF)
	 * 0x01000000 ... division by zero and invalid * sqrt(0) (DZ)
	 * 0x00800000 ... inexact result (NX)
	 * 0x00400000 ... NS bit
	 */

 {
         volatile uint32_t initval = 0x00400000;

         /* NOTE: load fsr means write to it */
         __asm__ __volatile__("ld [%0], %%fsr \n\t"
			   ::"r"(&initval));
 }
#endif



#if 1 /* CONFIG_LEON3 */

	leon3_flush();
	leon3_enable_icache();
	leon3_enable_dcache();
	leon3_enable_fault_tolerant();
	leon3_enable_snooping();
#endif /* CONFIG_LEON3 */

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

	 main_kernel_loop();
}


/**
 * @brief architecture setup entry point
 */

void setup_arch(void)
{
#if 1 /* CONFIG_LEON321 */

	leon3_flush();
	leon3_enable_icache();
	leon3_enable_dcache();
	leon3_enable_fault_tolerant();
	leon3_enable_snooping();
#endif /* CONFIG_LEON3 */

	mem_init();

	paging_init();

	BUG_ON(!kmem_init());

	reserve_kernel_stack();

	BUG_ON(stack_migrate(NULL, _kernel_stack_top));

	leon_irq_init();

	leon_edac_init();

	sparc_uptime_init();

	sparc_clockevent_init();

	/* XXX */
	sxi_dpu_setup_cpu_entry();

	smp_init();

	boot_cpus();

	sched_enable();
}
