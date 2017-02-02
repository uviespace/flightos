/**
 * @file arch/sparc/kernel/data_access_exception.c
 *
 */

#include <kernel/printk.h>
#include <mm.h>


void data_access_exception(void)
{
	mm_mmu_trap();
}
