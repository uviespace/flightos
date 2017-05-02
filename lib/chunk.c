/**
 * @file lib/chunk.c
 *
 * @ingroup chunk
 * @defgroup chunk Memory Chunk Allocator
 *
 * @brief a chunk-based memory allocator
 *
 *
 * This is a stepping stone to something like malloc()
 *
 * When chunk_alloc() is called, it attempts to first-fit-locate a buffer within
 * its tracked allocated chunks, splitting them into smaller chunks that track
 * their parent chunks. If it does not find a sufficiently large chunk, it
 * uses the user-supplied function to grab memory as needed from a higher-tier
 * memory manager. In case such a memory manager cannot provide buffers
 * exactly the requested size, an optional function real_alloc_size() may be
 * provided, so the chunk allocator can know the actual size of the allocated
 * block (e.g. from kernel/page.c)
 *
 * When freeing an allocation with chunk_free() and the chunk is the child of
 * another chunk, the reference counter of the parent is decremented and the
 * child is released and upmerged IF the child was the last allocation in the
 * parent's buffer, otherwise it is treated as any other available chunk.
 * If a top-level chunk's refernce counter is decremented to 0, it is released
 * back to the higher-tier memory manager.
 *
 * Note that the address verification in chunk_free() is weak, as it only
 * checks if it's start address and size are within the chunk of the parent.
 *
 * @todo add a function chunk_alloc_aligned() that allows aligned allocations
 *	 without wasting space. the function just grabs some memory and creates
 *	 a single chunk up to the alignement boundary and adds it to the "full"
 *	 pool, then returns the following (aligned) chunk.
 *
 * @example chunk_demo.c
 */

#include <unistd.h>
#include <errno.h>
#include <list.h>
#include <kernel/kernel.h>
#include <kernel/printk.h>

#include <chunk.h>


/**
 * The header of an alloccated memory chunk.
 *
 * this isn't partcicularly memory-efficient for small allocations...
 */

struct chunk {
	void	*mem;		/* the start of the data memory */

	size_t	size;		/* the allocated size of the chunk */
	size_t	free;		/* the free memory in the chunk */

	unsigned long refcnt;	/* number of references to this chunk */
	struct chunk *parent;	/* the parent chunk */
	struct chunk *child;	/* the youngest child chunk */
	struct chunk *sibling;	/* the left-hand (younger) child chunk */

	struct list_head node;
};




/**
 * @brief align a pointer to the alignment configured
 *
 * @param pool a struct chunk_pool
 *
 * @param p an address pointer
 *
 * @return the aligned pointer
 */

static inline void *chunk_align(struct chunk_pool *pool, void *p)
{
	return ALIGN_PTR(p, pool->align);
}


/**
 * @brief classify a chunk as full or empty
 *
 * @param pool a struct chunk_pool
 * @param c a struct chunk
 */

static void chunk_classify(struct chunk_pool *pool, struct chunk *c)
{
	/* if it can't fit at least one header, it's no good */
	if (c->free <= (sizeof(struct chunk) + pool->align))
		list_move_tail(&c->node, &pool->empty);
	else
		list_move_tail(&c->node, &pool->full);
}


/**
 * @brief configure data pointer and free bytes of a chunk
 *
 * @param pool a struct chunk_pool
 * @param c a struct chunk
 */

static void chunk_setup(struct chunk_pool *pool, struct chunk *c)
{
	/* the actual memory starts after struct chunk */
	c->mem = chunk_align(pool, (void *) (c + 1));
	/* set the allocatable size of the cunk */
	c->free = ((size_t) c + c->size) - (size_t) c->mem;
}


/**
 * @brief grab a new chunk of memory from the higher-tier memory manager
 *
 * @param pool a struct chunk_pool
 *
 * @param size the requested minimum size of the chunk
 *
 * @return a struct chunk or NULL on error
 */

static struct chunk *chunk_grab_new(struct chunk_pool *pool, size_t size)
{
	struct chunk *c;


	c = (struct chunk *) pool->alloc(size);

	if (!c)
		return NULL;


	/* if set, get actual chunk size */
	if (pool->real_alloc_size) {

		c->size = pool->real_alloc_size((void *) c);

		if (c->size < size) {
			pr_warn("CHUNK: got less bytes than expected from "
				"higher-tier memory manager\n");
			pool->free((void *) c);
			return NULL;
		}

	} else {
		c->size = size;
	}

	/* this is a new toplevel chunk, it's all alone */
	c->parent  = NULL;
	c->child   = NULL;
	c->sibling = NULL;

	/* we have no references yet */
	c->refcnt = 0;

	chunk_setup(pool, c);

	/* add new parent to full list by default */
	list_add_tail(&c->node, &pool->full);



	return c;
}


/**
 * @brief split of a new chunk as a child of an existing chunk
 *
 * @param pool a struct chunk_pool
 *
 * @param c a struct chunk that is the parent and can hold the child
 *
 * @param size the size of the new chunk
 *
 * @return a struct chunk or NULL on error
 */

static struct chunk *chunk_split(struct chunk_pool *pool,
				 struct chunk *c, size_t alloc_sz)
{
	struct chunk *new;


	if (c->free < alloc_sz)
		return NULL;

	/* this chunk is now a child of a higher-order chunk */
	new = (struct chunk *) c->mem;
	new->parent  = c;
	new->child   = NULL;
	new->sibling = c->child;
	new->refcnt  = 1;
	new->size    = alloc_sz;

	chunk_setup(pool, new);

	/* the new node will be in use, add to empty list */
	new->free = 0;
	list_add_tail(&new->node, &pool->empty);


	/* track the youngest child in the parent */
	c->child = new;
	c->refcnt++;

	/* align parent chunk to start of new memory subsegment */
	c->mem = chunk_align(pool, (void *) ((size_t) c->mem + alloc_sz));

	/* update free bytes with regard to actual alignment */
	c->free = ((size_t) c + c->size - (size_t) c->mem);

	chunk_classify(pool, c);

	return new;
}


/**
 * @brief allocate a chunk of memory
 *
 * @param pool a struct chunk_pool
 * @param size the number of bytes to allocate
 *
 * @return a pointer to a memory buffer or NULL on error
 */

void *chunk_alloc(struct chunk_pool *pool, size_t size)
{
	size_t alloc_sz;

	struct chunk *c = NULL;

	struct chunk *p_elem;



	if (!pool->alloc) {
		pr_err("CHUNK: error, no allocator supplied.\n");
		return NULL;
	}


	if (!size)
		return NULL;


	/* grab a large enough chunk to satisfy alignment and overhead needs */
	alloc_sz = (size_t) chunk_align(pool,
			(void *) (size + pool->align + sizeof(struct chunk)));

	list_for_each_entry(p_elem, &pool->full, node) {


		if (p_elem->free >= alloc_sz) {
			c = p_elem;
			break;
		}
	}

	/* no chunks, either list is empty or none had enough space */
	if (!c) {

		c = chunk_grab_new(pool, alloc_sz);

		if (!c) {
			pr_err("CHUNK: error, got no memory from allocator \n");
			return NULL;
		}
	}

	c = chunk_split(pool, c, alloc_sz);

	return c->mem;
}


/**
 * @brief free a chunk of memory
 *
 * @param pool a struct chunk_pool
 * @param addr the address of the buffer to return to the allocator
 *
 */

void chunk_free(struct chunk_pool *pool, void *addr)
{
	unsigned long chunk_addr;

	struct chunk *c;
	struct chunk *p;


	if (!addr)
		return;


	if (!pool->alloc) {
		pr_err("CHUNK: error, no de-allocator supplied.\n");
		return;
	}


	/* the start of the chunk is off by at most the size of the header minus
	 * the (alignment - 1)
	 */
	chunk_addr = (unsigned long) addr - sizeof(struct chunk) - pool->align;
	c = (struct chunk *) chunk_align(pool, (void *) chunk_addr);


	/* if the address of the data chunk does not coincide with what we just
	 * calculated, something is seriously wrong here
	 */
	BUG_ON((size_t) addr != (size_t) chunk_align(pool, (void *) (c + 1)));


	/* if this is a toplevel chunk, release it back to the higher-tier */
	if (!c->parent) {

		BUG_ON(c->refcnt);

		pool->free((void *) c);
		list_del(&c->node);

		return;
	}


	p = c->parent;

	/* weak check if this is even valid */
	if (addr < (void *) p) {
		pr_warn("CHUNK: invalid address %p supplied in call to %s\n",
			addr, __func__);
		return;
	}

	if (((size_t) c + c->size) > ((size_t) p + p->size)) {
		pr_warn("CHUNK: invalid address %p, or attempted double free "
			"in call to %s\n", addr, __func__);
	}

	/* If this he youngest child, merge it back and update the parent. */
	if (p->child == c) {

		p->child = c->sibling;
		p->refcnt--;

		/* align parent chunk */
		p->mem = chunk_align(pool, (void *) ((size_t) c));

		/* update free parent bytes with regard to actual alignment */
		p->free = ((size_t) p + p->size) - (size_t) c;

		/* make sure this will not fit the parent without overflowing
		 * (except for the obvious edge case that should never happen
		 * anyways), so we may detect a double-free
		 */
		c->size = p->size - c->size + 1;

		list_del(&c->node);

		if (!p->refcnt)
			chunk_free(pool, p->mem);

	} else {
		chunk_setup(pool, c);
		list_move_tail(&c->node, &pool->full);
	}
}


/**
 * @brief initialise a chunk pool
 *
 * @param pool a struct chunk pool
 * @param align the memory alignment of returned memory pointers
 * @param alloc a pointer to a higher-tier function that
 *	  gives us a chunk of memory to manage
 * @param free a pointer to a function that returns a chunk of memory to the
 *	  higher-tier memory manager
 * @param real_alloc_size a pointer to a function that tells us how large the
 *	  chunk returned by alloc() actually is. This may be NULL if alloc()
 *	  always returns a chunk the exact size we requested
 *
 * @note the alloc() function should at least return word-aligned addresses
 */

void chunk_pool_init(struct chunk_pool *pool,
		     unsigned long align,
		     void *(*alloc)(size_t size),
		     void  (*free)(void *addr),
		     size_t (*real_alloc_size)(void *addr))
{
	INIT_LIST_HEAD(&pool->full);
	INIT_LIST_HEAD(&pool->empty);

	pool->align = align;

	pool->alloc = alloc;
	pool->free  = free;

	pool->real_alloc_size = real_alloc_size;
}
