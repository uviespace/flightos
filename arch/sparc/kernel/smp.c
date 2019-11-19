/**
 * @file arch/sparc/kernel/smp.c
 *
 * @ingroup sparc
 * @brief implements architecture-specific @ref kernel_smp interface
 */

#include <kernel/smp.h>
#include <irq.h>
#include <asm/io.h>
#include <asm/irq.h>

#ifdef CONFIG_LEON3
#include <asm/leon.h>
#endif

#ifdef CONFIG_LEON4
#include <asm/leon.h>
#endif


int smp_cpu_id(void)
{

#if defined(CONFIG_LEON4) || defined(CONFIG_LEON3)
	return leon3_cpuid();
#endif
	/* everything else */
	return 0;
}


void smp_send_reschedule(int cpu)
{
#if defined(CONFIG_LEON4) || defined(CONFIG_LEON3)
	/* trigger reschedule via forced IRQMP extended interrupt */
	/* TODO sanity check for cpu id */
	leon_force_irq(LEON3_IPIRQ, cpu);
#endif /* CONFIG_LEON3 */
}


void smp_init(void)
{
#if defined(CONFIG_LEON4) || defined(CONFIG_LEON3)

	int i;

	for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++)
		leon_enable_irq(LEON3_IPIRQ, i);

#endif /* CONFIG_LEON3 */
}
