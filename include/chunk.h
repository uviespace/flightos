/**
 * @file include/chunk.h
 */


#ifndef _CHUNK_H_
#define _CHUNK_H_


struct chunk_pool {
	struct list_head full;
	struct list_head empty;

	unsigned long align;

	void *(*alloc)(size_t size);
	void  (*free)(void *addr);
	size_t (*real_alloc_size)(void *addr);
};

void *chunk_alloc(struct chunk_pool *pool, size_t size);

void chunk_free(struct chunk_pool *pool, void *addr);

void chunk_pool_init(struct chunk_pool *pool,
		     unsigned long align,
		     void *(*alloc)(size_t size),
		     void  (*free)(void *addr),
		     size_t (*real_alloc_size)(void *addr));

#endif /* _CHUNK_H_ */
