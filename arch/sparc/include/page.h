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


#if defined (CONFIG_SPARC_INIT_PAGE_MAP_MAX_ENTRIES)
#define INIT_PAGE_MAP_MAX_ENTRIES CONFIG_SPARC_INIT_PAGE_MAP_MAX_ENTRIES
#else
#define INIT_PAGE_MAP_MAX_ENTRIES 1
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
