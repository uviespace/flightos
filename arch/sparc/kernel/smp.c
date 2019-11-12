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


int smp_cpu_id(void)
{
#ifdef CONFIG_LEON3
	return leon3_cpuid();
#else /* !CONFIG_LEON3 */
	return 0;
#endif /* CONFIG_LEON3 */
}


void smp_send_reschedule(int cpu)
{
#ifdef CONFIG_LEON3
	/* trigger reschedule via forced IRQMP extended interrupt */
	/* TODO sanity check for cpu id */
	leon_force_irq(LEON3_IPIRQ, cpu);
#endif /* CONFIG_LEON3 */
}


void smp_init(void)
{
#ifdef CONFIG_LEON3

	/* XXX need number of CPUs here */
	leon_enable_irq(LEON3_IPIRQ, 0);
	leon_enable_irq(LEON3_IPIRQ, 1);

#endif /* CONFIG_LEON3 */
}
