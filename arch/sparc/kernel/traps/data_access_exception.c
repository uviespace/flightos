/**
 * @file arch/sparc/kernel/data_access_exception.c
 *
 */

#include <kernel/printk.h>

#ifdef CONFIG_MMU
#include <mmu.h>
#endif /* CONFIG_MMU */


/**
 * @brief handle data access exceptions
 *
 * @note delegates to the MMU trap handler if CONFIG_MMU is enabled
 */

void data_access_exception(void)
{
#ifdef CONFIG_MMU
	mm_mmu_trap();
#endif /* CONFIG_MMU */
}
