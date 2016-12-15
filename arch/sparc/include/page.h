/**
 * @file arch/sparc/include/page.h
 *
 * @brief page memory definitions for MMU operation
 */

#ifndef _SPARC_PAGE_H_ 
#define _SPARC_PAGE_H_ 

#include <kernel/kernel.h>

#include <mm.h>

/* the page size in the SRMMU is 4kib */
#define PAGE_SHIFT   12
#define PAGE_SIZE    (0x1UL << PAGE_SHIFT)
#define PAGE_MASK    (~(PAGE_SIZE - 1))

/* align address to the (next) page boundary */
#define PAGE_ALIGN(addr)	ALIGN(addr, PAGE_SIZE)

#define PFN_PHYS(x)	((unsigned long)((x) << PAGE_SHIFT))
#define PHYS_PFN(x)	((unsigned long)((x) >> PAGE_SHIFT))


#if defined(CONFIG_PAGE_OFFSET)
#define PAGE_OFFSET	0xf0000000
#endif

extern unsigned long phys_base;
extern unsigned long pfn_base;

#if defined(CONFIG_PAGE_OFFSET)
#define __pa(x)		((unsigned long)(x) - PAGE_OFFSET + phys_base)
#define __va(x)		((void *)((unsigned long) (x) - phys_base + PAGE_OFFSET))
#else
#define __pa(x)		(x)
#define __va(x)		(x)
#endif /* CONFIG_PAGE_OFFSET */

struct pg_data {
	int x;
};


extern struct pg_data page_mem[SPARC_PHYS_BANKS+1];


unsigned long init_page_map(struct pg_data *pg,
			    unsigned long start_pfn,
			    unsigned long end_pfn);


#endif /*_SPARC_PAGE_H_*/
