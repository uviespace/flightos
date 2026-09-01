/**
 * @file include/kernel/mm.h
 *
 * @ingroup kmem
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
	unsigned long    base;		/*!< base address of the memory pool */
	unsigned long    max_order;	/*!< maximum order (i.e. pool size)  */
	unsigned long    min_order;	/*!< block granularity		    */
	unsigned long    n_blks;	/*!< number of managed blocks	    */
	unsigned long    alloc_blks;	/*!< number of allocated blocks	    */
	unsigned char    *alloc_order;	/*!< the allocated order of a block  */
	unsigned long    *blk_free;	/*!< per-block allocation bitmap    */
	struct list_head *block_order;  /*!< anchor for unused blocks	    */
};


/**
 * @brief allocate memory from a buddy memory pool
 * @param mp: the memory pool to allocate from
 * @param size: number of bytes to allocate
 * @return pointer to allocated memory, or NULL on failure
 */
void *mm_alloc(struct mm_pool *mp, size_t size);

/**
 * @brief free memory back to a buddy memory pool
 * @param mp: the memory pool to return memory to
 * @param addr: pointer to memory previously allocated by mm_alloc
 */
void mm_free(struct mm_pool *mp, const void *addr);

/**
 * @brief get the number of unallocated blocks in a pool
 * @param mp: the memory pool to query
 * @return number of free blocks
 */
unsigned long mm_unallocated_blocks(struct mm_pool *mp);

/**
 * @brief get the number of allocated blocks in a pool
 * @param mp: the memory pool to query
 * @return number of allocated blocks
 */
unsigned long mm_allocated_blocks(struct mm_pool *mp);

/**
 * @brief get the total free memory across all pools
 * @return total free bytes available
 */
size_t mm_free_bytes(void);

/**
 * @brief check if an address belongs to a specific memory pool
 * @param mp: the memory pool to check
 * @param addr: the address to verify
 * @return true if addr is within the pool, false otherwise
 */
bool mm_addr_in_pool(struct mm_pool *mp, void *addr);

/**
 * @brief get the block size for an address in a pool
 * @param mp: the memory pool containing the address
 * @param addr: the address to query
 * @return the allocation block size in bytes
 */
unsigned long mm_block_size(struct mm_pool *mp, const void *addr);

/**
 * @brief initialize a buddy memory pool
 * @param mp: the pool structure to initialize
 * @param base: base address of the memory region
 * @param pool_size: total size of the memory region in bytes
 * @param granularity: minimum allocation block size in bytes
 * @return 0 on success, negative error code on failure
 */
int mm_init(struct mm_pool *mp, void *base,
	    size_t pool_size, size_t granularity);

/**
 * @brief release all resources associated with a memory pool
 * @param mp: the pool to destroy
 */
void mm_exit(struct mm_pool * mp);

/**
 * @brief dump pool statistics for debugging
 * @param mp: the pool to display statistics for
 */
void mm_dump_stats(struct mm_pool *mp);


#endif /* _KERNEL_MM_H_ */
