/**
 * @file arch/sparc/kernel/init.c
 */

#include <mm.h>

void paging_init(void)
{
	bootmem_init();
}
