/**
 * @file arch/sparc/kernel/mm.c
 *
 * @ingroup sparc
 */

#include <mm.h>

#include <kernel/kernel.h>
#include <kernel/printk.h>

#include <page.h>
#include <cpu_type.h>

/* things we need statically allocated in the image (i.e. in .bss)
 * at boot
 */

unsigned long phys_base;
unsigned long pfn_base;

struct sparc_physical_banks sp_banks[SPARC_PHYS_BANKS + 1];
