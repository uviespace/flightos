/**
 * @file include/kernel/page.h
 *
 */

#ifndef _KERNEL_PAGE_H_ 
#define _KERNEL_PAGE_H_


#include <kernel/mm.h>


struct page_map_node {
	struct mm_pool *pool;
	unsigned long mem_start;
	unsigned long mem_end;
	struct list_head node;
};


#if defined(CONFIG_PAGE_MAP_MOVE_NODE_AVAIL_THRESH)
#define PAGE_MAP_MOVE_NODE_AVAIL_THRESH CONFIG_PAGE_MAP_MOVE_NODE_AVAIL_THRESH
#else
#define PAGE_MAP_MOVE_NODE_AVAIL_THRESH 1 
#endif

unsigned long page_map_get_chunk_size(void *addr);
void page_print_mm_alloc(void);

int page_map_init(struct page_map_node **pg,
		  unsigned long start, unsigned long end,
		  unsigned long page_size);

void page_map_set_map(struct page_map_node **pg);

int page_map_add(unsigned long start, unsigned long end,
		 unsigned long page_size);

void *page_map_reserve_chunk(size_t size);

void *page_alloc(void);
void page_free(void *page);


#endif /* _KERNEL_PAGE_H_ */
