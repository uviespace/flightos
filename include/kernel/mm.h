/**
 * @file include/kernel/mm.h
 *
 * @ingroup buddy_mm
 */

#ifndef _KERNEL_MM_H_
#define _KERNEL_MM_H_

#include <stddef.h>
#include <stdbool.h>

#include <list.h>
#include <compiler.h>
#include <kernel/bitops.h>


#define MM_NUM_BLOCKS_TRACKABLE(order_max, order_min) \
	((1UL << order_max) / (1UL << order_min))

#define MM_BITMAP_LEN(order_max, order_min) \
	(BITS_TO_LONGS(MM_NUM_BLOCKS_TRACKABLE(order_max, order_min)) + 1)

/**
 * the buddy memory pool
 */
struct mm_pool {
	unsigned long    base;		/**! base address of the memory pool */
	unsigned long    max_order;	/**! maximum order (i.e. pool size)  */
	unsigned long    min_order;	/**! block granularity		    */
	unsigned long    n_blks;	/**! number of managed blocks	    */
	unsigned long    alloc_blks;	/**! number of allocated blocks	    */
	unsigned char    *alloc_order;	/**! the allocated order of a block  */
	unsigned long    *blk_free;	/**! per-block allocation bitmap    */
	struct list_head *block_order;  /**! anchor for unused blocks	    */
};


void *mm_alloc(struct mm_pool *mp, size_t size);

void mm_free(struct mm_pool *mp, const void *addr);

unsigned long mm_unallocated_blocks(struct mm_pool *mp);
unsigned long mm_allocated_blocks(struct mm_pool *mp);

size_t mm_free_bytes(void);

bool mm_addr_in_pool(struct mm_pool *mp, void *addr);

unsigned long mm_block_size(struct mm_pool *mp, const void *addr);

int mm_init(struct mm_pool *mp, void *base,
	    size_t pool_size, size_t granularity);

void mm_exit(struct mm_pool * mp);

void mm_dump_stats(struct mm_pool *mp);


#endif /* _KERNEL_MM_H_ */

