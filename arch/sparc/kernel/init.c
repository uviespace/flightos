/**
 * @file arch/sparc/kernel/init.c
 */

#include <mm.h>
#include <srmmu.h>

void paging_init(void)
{
	bootmem_init();
	mm_mmu_paging_init();
}
