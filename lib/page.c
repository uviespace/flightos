/**
 * @file kernel/page.c
 * @ingroup page
 * @defgroup page Page Map Manager
 *
 * @brief a page map manager and allocator
 * 
 * Manages a memory pool managed by @ref buddy_mm 
 *
 * @note this only ever uses one page map at a time, so if you want to remap
 *	 boot memory, define a new map or just expand the number of nodes in the
 *	 page map, first copy the old map and call page_map_set_ref()
 */


#include <page.h>

#include <kernel/err.h>
#include <kernel/mm.h>
#include <kernel/printk.h>
#include <kernel/kernel.h>

#include <string.h>


#define PG_SIZE(map)    (0x1UL << map->pool->min_order)

/* align address to the (next) page boundary */
#define PG_ALIGN(addr, map)	ALIGN(addr, PG_SIZE(map))

/* reference to the page map, last entry must be NULL */
static struct page_map_node **page_mem;

/* empty/busy pool lists */
static struct list_head page_map_list_full;
static struct list_head page_map_list_empty;


/**
 * @brief set the map that is used by page_alloc() and page_free() etc.
 *
 * @param pg		a page map
 * @param nodes		the number of nodes the map can hold
 *
 */

void page_map_set_map(struct page_map_node **pg)
{
	page_mem = pg;

	INIT_LIST_HEAD(&page_map_list_full);
	INIT_LIST_HEAD(&page_map_list_empty);


	/* consider all as full at the beginning */
	while ((*pg))
		list_add_tail(&(*pg++)->node, &page_map_list_full);
}


/**
 * @brief add a new mapping
 *
 * @param start		the start address of the memory section
 * @param end		the end address of the memory section
 * @param page_size	the page size granularity
 *
 * @return 0 on success, -ENOMEM on error
 *
 */

int page_map_add(unsigned long start, unsigned long end,
		 unsigned long page_size)
{
	size_t mem_size;

	struct page_map_node **pg = page_mem;


	if (!pg) {
		pr_err("PAGE MEM: %s(): no page map configured\n", __func__);
		goto error;
	}

	if (end < start)
		goto error;

	if ((end - start) < page_size)
		goto error;

	if (!page_size)
		goto error;


	mem_size = (size_t) end - start;

	/* search for first empty pool entry (max_order == 0)*/
	do {
		if (!(*pg)->pool->max_order)
			break;

		/* check for overlapping configurations */
		if ((*pg)->mem_start <= start)
			if ((*pg)->mem_end > start)
				goto overlap;

		if ((*pg)->mem_end >= end)
			if ((*pg)->mem_start < end)
				goto overlap;

	} while ((*(++pg)));


	if (!(*pg)) {
		pr_err("PAGE MEM: map space exceeded, cannot add map\n");
		goto error;
	}
	if (mm_init((*pg)->pool, (void *) start, mem_size, page_size))
		goto error;

	(*pg)->mem_start = start;
	(*pg)->mem_end = end;

	list_add_tail(&(*pg)->node, &page_map_list_full);

	return 0;

error:
	return -ENOMEM;
overlap:
	printk("PAGE MEM: overlapping existing memory range 0x%08x 0x%08x found for request "
	       "0x%08x 0x%08x cannot add map\n", (*pg)->mem_start, (*pg)->mem_end, start, end);
	return -EINVAL;
}



/**
 * @brief initialise a page map
 *
 * @param pg		a page map
 * @param start		the start address of the initial memory section
 * @param end		the end address of the initial memory section
 * @param page_size	the page size granularity
 *
 * @return 0 on success, -ENOMEM on error
 *
 * @note the page map store is assumed to be set to zero
 *
 * @warn do not forget to configure all the pool storage pointers...
 */

int page_map_init(struct page_map_node **pg_map,
		  unsigned long start, unsigned long end,
		  unsigned long page_size)
{
	size_t mem_size;

	struct page_map_node *pg = pg_map[0];


	if (!pg)
		goto error;

	if (end < start)
		goto error;

	mem_size = (size_t) end - start;

	if (mm_init(pg->pool, (void *) start, mem_size, page_size))
		goto error;

	pg->mem_start = start;
	pg->mem_end = end;

	page_map_set_map(pg_map);


	return 0;

error:
	return -ENOMEM;
}


/**
 * @brief reserve an arbitrarily sized chunk of memory, but track it in the
 *	  page mapper
 *
 * @param mp a struct mm_pool
 * @param size the size to allocate
 *
 * @return void pointer on success, NULL on error
 *
 * @note this function can only a valid pointer if enough contiguous blocks
 *	 are available in the memory manager!
 *
 * @note To reserve your boot memory/image, call this function before any page
 *	 in the initial memory segment has been allocated, because only then the
 *	 free block header has not yet been written and the segment is
 *	 completely untouched. See notes in mm.c for more info.
 *	 This is of course inefficient, if your image/boot memory does not
 *	 coincide with the start of the memory bank. In this case you might want
 *	 to consider re-adding the free segment before your boot memory back
 *	 to the page map. In that case, make sure the allocation is never
 *	 released. Make sure you configure extra ram banks if needed.
 *
 * @note the reserved block is at least size bytes
 */

void *page_map_reserve_chunk(size_t size)
{
	void *mem = NULL;

	struct page_map_node **pg = page_mem;

	if (!pg) {
		pr_err("PAGE MEM: %s no page map configured\n", __func__);
		goto exit;
	}

	/* do NOT care for the empty/full lists, just find the first pool with
	 * a sufficiently large block
	 */
	do {
		mem = mm_alloc((*pg)->pool, size);

		if (mem)
			break;
	} while ((*(++pg)));

exit:
	return mem;
}


/**
 * @brief get the size of the chunk for an address
 *
 * @param addr the (page) address pointer
 *
 * @return the size of the chunk, or 0 on error or if not found in pool
 */

unsigned long page_map_get_chunk_size(void *addr)
{
	unsigned long size = 0;

	struct page_map_node *p_elem;
	struct page_map_node *p_tmp;


	if (!page_mem) {
		pr_err("PAGE MEM: %s no page map configured\n", __func__);
		goto exit;
	}

	if (!addr) {
		pr_info("PAGE MEM: NULL pointer in call to %s from %p\n",
			__func__, __caller(0));
		goto exit;
	}

	list_for_each_entry_safe(p_elem, p_tmp, &page_map_list_empty, node) {
		if (mm_addr_in_pool(p_elem->pool, addr)) {
			size = mm_block_size(p_elem->pool, addr);
			goto exit;
		}
	}

	list_for_each_entry_safe(p_elem, p_tmp, &page_map_list_full, node) {
		if (mm_addr_in_pool(p_elem->pool, addr)) {
			size = mm_block_size(p_elem->pool, addr);
			goto exit;
		}
	}

exit:
	return size;
}






/**
 * @brief allocates a page by trying all configured banks until one is found
 *
 * @return NULL on error, address to page on success
 */

void *page_alloc(void)
{
	void *page = NULL;

	struct page_map_node *p_elem;
	struct page_map_node *p_tmp;


	if (!page_mem) {
		pr_err("PAGE MEM: %s no page map configured\n", __func__);
		goto exit;
	}

	list_for_each_entry_safe(p_elem, p_tmp, &page_map_list_full, node) {

		page = mm_alloc(p_elem->pool, PG_SIZE(p_elem));

		if (!page) {
			list_move_tail(&p_elem->node, &page_map_list_empty);
			pr_debug("PAGE MEM: mapping %p move to empty list\n",
				 p_elem);
		}
		else
			break;
	}


#ifdef CONFIG_PAGE_MAP_CHECK_PAGE_ALIGNMENT

	if (!p_elem)
		goto exit;

	if ((unsigned long) page != (PG_ALIGN((unsigned long) page, p_elem))) {
		pr_err("PAGE MAP: page at %p allocated from memory manager %p "
		       "is not aligned to the configured page size");
		mm_free(p_elem->pool, page);
		page = NULL;
	}
#endif

exit:
	return page;
}


/**
 * @brief free a page
 *
 * @param page the page address pointer
 *
 * @note nested mappings should be caught by mm_addr_in_pool() check
 */

void page_free(void *page)
{
	struct page_map_node *p_elem;
	struct page_map_node *p_tmp;


	if (!page_mem) {
		pr_err("PAGE MEM: %s no page map configured\n", __func__);
		return;
	}

	if (!page) {
		pr_info("PAGE MEM: NULL pointer in call to page_free from %p\n",
			__caller(0));
			return;
	}

	/* first check empty list */
	list_for_each_entry_safe(p_elem, p_tmp, &page_map_list_empty, node) {

		if (mm_addr_in_pool(p_elem->pool, page)) {

			mm_free(p_elem->pool, page);

			/* always move to the head of the list, worst case it is
			 * followed by a node that holds free blocks between 0
			 * and threshold
			 */
			if (mm_unallocated_blocks(p_elem->pool)
			    >= PAGE_MAP_MOVE_NODE_AVAIL_THRESH) {
				pr_debug("PAGE MEM: mapping %p move to full "
					 "list\n", p_elem);
				list_move_tail(&p_elem->node,
					  &page_map_list_full);
			}

			return;
		}
	}

	list_for_each_entry_safe(p_elem, p_tmp, &page_map_list_full, node) {
		if (mm_addr_in_pool(p_elem->pool, page)) {
			mm_free(p_elem->pool, page);
			return;
		}
	}
}

void page_print_mm_alloc(void)
{
#ifdef CONFIG_MM_DEBUG_DUMP
	struct page_map_node **pg = page_mem;

	do {
		mm_dump_stats((*pg)->pool);

	} while ((*(++pg)));
#endif /* CONFIG_MM_DEBUG_DUMP */
}
