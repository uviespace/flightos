/**
 * @file include/kernel/page.h
 *
 */

#ifndef _KERNEL_PAGE_H_ 
#define _KERNEL_PAGE_H_


#include <kernel/mm.h>


struct page_map_node {
	struct mm_pool *pool;
	struct list_head node;
};


#if defined(CONFIG_PAGE_MAP_MOVE_NODE_AVAIL_THRESH)
#define PAGE_MAP_MOVE_NODE_AVAIL_THRESH CONFIG_PAGE_MAP_MOVE_NODE_AVAIL_THRESH
#else
#define PAGE_MAP_MOVE_NODE_AVAIL_THRESH 1 
#endif

#endif /* _KERNEL_PAGE_H_ */
