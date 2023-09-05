/**
 * @file arch/sparc/include/page.h
 *
 * @brief page memory definitions for MMU operation
 */

#ifndef _SPARC_PAGE_H_
#define _SPARC_PAGE_H_

#include <kernel/kernel.h>
#include <kernel/page.h>
#include <kernel/mm.h>

#include <mm.h>
#include <mmu.h>

/* the page size in the SRMMU is 4kib */
#define PAGE_SHIFT   12
#define PAGE_SIZE    (0x1UL << PAGE_SHIFT)
#define PAGE_MASK    (~(PAGE_SIZE - 1))

/* align address to the (next) page boundary */
#define PAGE_ALIGN(addr)	ALIGN(addr, PAGE_SIZE)
#define PAGE_ALIGN_PTR(addr)	PTR_ALIGN(addr, PAGE_SIZE)

#define PFN_PHYS(x)	((unsigned long)((x) << PAGE_SHIFT))
#define PHYS_PFN(x)((unsigned long)((x) >> PAGE_SHIFT))



/**
 * Reserved memory ranges used in mappings:
 *
 *  - everything above HIGHMEM_START is 1:1 mapped through the MMU and marks
 *    the per-thread allocatable memory.
 *  - LOWMEM_RESERVED is the reserved lower address range
 *  - VMALLOC_{START, END} define the boundaries of mapped virtual pages
 *
 * TODO: make this configurable via make config
 * The values are selected such that:
 *
 *	 1. the lowest 16M are reserved
 *	 2. highmem starts at 0x40000000, this is typically the lowest (RAM)
 *	    address in use on a LEON platform (usually internal static RAM).
 *	 3. the above boundaries are selected under the condition that the
 *	    kernel is not bootstrapped, i.e. an initial MMU context is not set
 *	    up by a stage 1 loader, which then copies and starts the kernel
 *	    from some location.
 */

#define HIGHMEM_START	0x20000000 /* XXX need iommu or lowmem area.. (for SXI, FPGA ADDR. */
#define LOWMEM_RESERVED 0x01000000

#define VMALLOC_START	(LOWMEM_RESERVED)
#define VMALLOC_END	(HIGHMEM_START - 1)


extern unsigned long phys_base;
extern unsigned long pfn_base;

#if defined(CONFIG_MMU)
#define __pa(x)		(mm_get_physical_addr(x))
#define __va(x)		(0)	/* not implemented */
#else
#define __pa(x)		(x)
#define __va(x)		(x)
#endif /* CONFIG_PAGE_OFFSET */


#if defined (CONFIG_SPARC_INIT_PAGE_MAP_MAX_ENTRIES)
#define INIT_PAGE_MAP_MAX_ENTRIES CONFIG_SPARC_INIT_PAGE_MAP_MAX_ENTRIES
#else
#define INIT_PAGE_MAP_MAX_ENTRIES 0
#endif

extern struct mm_pool  mm_init_page_pool;
extern struct page_map_node  mm_init_page_node;
extern struct page_map_node *mm_init_page_map[INIT_PAGE_MAP_MAX_ENTRIES + 1];

#define MEM_PAGE_MAP		(mm_init_page_map)
#define MEM_PAGE_NODE(x)	(((x) <= INIT_PAGE_MAP_MAX_ENTRIES) ? \
				 &MEM_PAGE_MAP[(x)] : NULL)

int page_map_init(struct page_map_node **pg,
		  unsigned long start, unsigned long end,
		  unsigned long page_size);

void page_map_set_map(struct page_map_node **pg);

int page_map_add(unsigned long start, unsigned long end,
		 unsigned long page_size);

void *page_map_reserve_chunk(size_t size);

void *page_alloc(void);
void page_free(void *page);


#endif /*_SPARC_PAGE_H_*/
