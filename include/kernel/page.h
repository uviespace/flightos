/**
 * @file include/kernel/page.h
 *
 * @ingroup kmem
 * @ingroup page
 */

#ifndef _KERNEL_PAGE_H_ 
#define _KERNEL_PAGE_H_


#include <kernel/mm.h>


/**
 * @brief page map node linking a memory pool to a physical memory range
 */
struct page_map_node {
	struct mm_pool *pool;		/*!< the buddy memory pool for this range */
	unsigned long mem_start;	/*!< start address of the memory range */
	unsigned long mem_end;		/*!< end address of the memory range */
	struct list_head node;		/*!< node in the page map list */
};


#if defined(CONFIG_PAGE_MAP_MOVE_NODE_AVAIL_THRESH)
#define PAGE_MAP_MOVE_NODE_AVAIL_THRESH CONFIG_PAGE_MAP_MOVE_NODE_AVAIL_THRESH
#else
#define PAGE_MAP_MOVE_NODE_AVAIL_THRESH 1 
#endif

/**
 * @brief get the allocation chunk size for an address
 * @param addr: the address to query
 * @return the chunk size in bytes
 */
unsigned long page_map_get_chunk_size(void *addr);

/**
 * @brief print page allocator memory usage for debugging
 */
void page_print_mm_alloc(void);

/**
 * @brief initialize a page map node with a new memory range
 * @param pg: pointer to the page map node pointer to initialize
 * @param start: start address of the memory range
 * @param end: end address of the memory range
 * @param page_size: page size for the allocator
 * @return 0 on success, negative error code on failure
 */
int page_map_init(struct page_map_node **pg,
		  unsigned long start, unsigned long end,
		  unsigned long page_size);

/**
 * @brief activate a page map node for allocations
 * @param pg: pointer to the page map node to activate
 */
void page_map_set_map(struct page_map_node **pg);

/**
 * @brief add a memory range to the page allocator
 * @param start: start address of the memory range
 * @param end: end address of the memory range
 * @param page_size: page size for the allocator
 * @return 0 on success, negative error code on failure
 */
int page_map_add(unsigned long start, unsigned long end,
		 unsigned long page_size);

/**
 * @brief reserve a contiguous chunk of memory from the page map
 * @param size: number of bytes to reserve
 * @return pointer to reserved memory, or NULL on failure
 */
void *page_map_reserve_chunk(size_t size);

/**
 * @brief allocate a single page
 * @return pointer to the allocated page, or NULL on failure
 */
void *page_alloc(void);

/**
 * @brief free a single page
 * @param page: pointer to the page to free
 */
void page_free(void *page);


#endif /* _KERNEL_PAGE_H_ */
