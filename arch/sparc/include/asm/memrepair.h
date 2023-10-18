/**
 * @file arch/sparc/include/memrepair.h
 */

#ifndef _ARCH_SPARC_ASM_MEMREPAIR_H_
#define _ARCH_SPARC_ASM_MEMREPAIR_H_

#include <asm/io.h>
#include <asm/spinlock.h>


/*
 * Irq masking is used to protect the re-write repair cycle within the
 * bounds of the CPU, as there is no 32bit compare-and-swap atomic in the
 * SPARC v8 instrution set. Be warned that it is not safe from memory
 * interaction from (the) other CPU(s) for the same reason. If necessary,
 * guards could be inserted to force other CPUs to pause execution via a
 * dedicated synchronisation mechanism, but that is potentially expensive
 * and highly detrimental when it comes to real-time scheduling
 * In any case, this can never be safe from DMA unless all IP cores are deactivated.
 *
 * Typical use cases will involve memory that is updated at high frequency,
 * e.g. for data processing and is therefore unlikely to encounter bit errors.
 * * Critical sections that are rarely or never written (such as program code)
 * will not suffer from concurrency issues. The same more or less applies to
 * stack memory, which is either re-written at high frequency and thus virtually
 * impervious to bit errors as well.
 */

#define __mem_repair __mem_repair
static void mem_repair(void *addr)
{
	uint32_t psr;
	uint32_t tmp;

	psr = spin_lock_save_irq();
	tmp = ioread32be(addr);
	iowrite32be(tmp, addr);
	spin_lock_restore_irq(psr);
}

#endif	/* _ARCH_SPARC_ASM_MEMREPAIR_H_ */
