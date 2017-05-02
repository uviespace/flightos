/**
 * @file arch/sparc/kernel/init.c
 *
 * @ingroup sparc
 *
 * @brief initalises paging
 */

#include <mm.h>

#ifdef CONFIG_MMU
#include <mmu.h>
#endif	/* CONFIG_MMU */

void paging_init(void)
{
	bootmem_init();
#ifdef CONFIG_MMU
	mm_mmu_paging_init();
#endif	/* CONFIG_MMU */
}
