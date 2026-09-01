/**
 * @file include/chunk.h
 *
 * @ingroup kmem
 * @ingroup chunk
 */


#ifndef _CHUNK_H_
#define _CHUNK_H_

#include <list.h>

/**
 * @brief chunk pool structure managing fixed-size allocation chunks
 */
struct chunk_pool {
	struct list_head full;		/*!< list of fully used chunks */
	struct list_head empty;		/*!< list of empty chunks */

	unsigned long align;		/*!< alignment requirement for chunks */

	void *(*alloc)(size_t size);	/*!< allocator function for chunks */
	void  (*free)(void *addr);	/*!< deallocator function for chunks */
	size_t (*real_alloc_size)(void *addr); /*!< returns the true allocated size */
};

/**
 * @brief allocate a chunk of memory from the pool
 * @param pool: the chunk pool
 * @param size: requested size in bytes
 * @return pointer to allocated chunk, or NULL on failure
 */
void *chunk_alloc(struct chunk_pool *pool, size_t size);

/**
 * @brief free a chunk back to the pool
 * @param pool: the chunk pool
 * @param addr: pointer to the chunk to free
 */
void chunk_free(struct chunk_pool *pool, void *addr);

/**
 * @brief initialize a chunk pool
 * @param pool: the chunk pool to initialize
 * @param align: memory alignment for chunks
 * @param alloc: allocator callback
 * @param free: deallocator callback
 * @param real_alloc_size: callback returning the true allocated size of a chunk
 */
void chunk_pool_init(struct chunk_pool *pool,
		     unsigned long align,
		     void *(*alloc)(size_t size),
		     void  (*free)(void *addr),
		     size_t (*real_alloc_size)(void *addr));

#endif /* _CHUNK_H_ */
