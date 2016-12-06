/**
 * @file arch/sparc/kernel/mm.c
 */

#include <mm.h>


unsigned long phys_base;
unsigned long pfn_base;

struct sparc_physical_banks sp_banks[SPARC_PHYS_BANKS+1];
