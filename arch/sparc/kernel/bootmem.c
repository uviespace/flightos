/**
 * @file arch/sparc/kernel/bootmem.c
 *
 * @ingroup sparc
 *
 * @brief uses @ref chunk to manage the physical RAM banks
 *
 * This sets up a buddy system memory manager via @ref chunk that 
 * handles the physical RAM banks. The kernel image RAM section itself is
 * reserved, and a more fine-grained boot memory allocator is set up that can be
 * used to reserve memory for low-level kernel objects such as MMU tables.
 *
 * Note that because we don't use a two-stage bootstrap process at this time,
 * we will 1:1 map the relevant physical memory bank addresses through the MMU,
 * so the low level objects may be used without a translation step for now,
 * but we will migrate to a new kernel stack base at (2^32 - 1) VA once we
 * have set up the MMU.
 *
 * Because of the above, user space memory will be mapped below the
 * boot address of the kernel image, i.e. 0x40000000 for typical LEON hardware,
 * since we want to keep our kernel context in the MMU at all times.
 */

#include <page.h>
#include <stack.h>

#include <string.h>

#include <kernel/printk.h>
#include <kernel/kernel.h>

#include <chunk.h>


/* the pool we use for our boot memory allocator */
static struct chunk_pool phys_mem_pool;


/**
 * @brief boot memory allocator wrapper
 * @param size the number of bytes to allocate
 *
 * @return a pointer to a buffer or NULL on error
 */

static void *bootmem_alloc_internal(size_t size)
{
#ifndef CONFIG_SPARC_BOOTMEM_REQUEST_NEW_ON_DEMAND
	static int blocked;

	if (blocked) {
		pr_crit("BOOTMEM: out of memory\n");
		return NULL;
	}
	blocked = 1;
#endif

#if (BOOTMEM_CHUNKSIZE > PAGE_SIZE)
	return page_map_reserve_chunk(BOOTMEM_CHUNKSIZE);
#else
	return page_alloc();
#endif
}


/**
 * @brief boot memory free wrapper
 * @param addr the address to free
 */

static void bootmem_free_internal(void *ptr)
{
	page_free(ptr);
}

/**
 * @brief finds the actual size of an allocated buffer
 * @param addr the address to look up
 *
 * @return the buffer size, or 0 if invalid address or not found
 */

static size_t bootmem_get_alloc_size(void *ptr)
{
	return page_map_get_chunk_size(ptr);
}


/**
 * @brief allocate a buffer from the boot memory allocator
 *
 * @param size the number of bytes to allocate
 *
 * @return a pointer to a buffer or NULL on error
 */

void *bootmem_alloc(size_t size)
{
	void *ptr;

	if (!size)
		return NULL;

	ptr = chunk_alloc(&phys_mem_pool, size);

	if (!ptr) {
		pr_emerg("BOOTMEM: allocator out of memory\n");
		BUG();
	}

	return ptr;
}


/**
 * @brief release a buffer to the boot memory allocator
 *
 * @param addr the address to release
 */

void bootmem_free(void *ptr)
{
	chunk_free(&phys_mem_pool, ptr);
}


/**
 * @brief initialise the boot memory
 */

void bootmem_init(void)
{
	int i;

	int ret;

	unsigned long base_pfn;
	unsigned long start_pfn;
	unsigned long start_img_pfn;
	unsigned long end_pfn = 0UL;
	unsigned long mem_size;

	unsigned long node = 0;

	struct page_map_node **pg_node;


	pr_info("BOOTMEM: start of program image at %p\n", _start);
	pr_info("BOOTMEM:   end of program image at %p\n", _end);


	/* lowest page frame number coincides with page aligned address
	 * start symbol in image, which hopefully coincides with the start
	 * of the RAM we are running from.
	 */
	start_img_pfn = PAGE_ALIGN((unsigned long) &_start);

	/* start allocatable memory with page aligned address of last symbol in
	 * image, everything before will be reserved
	 */
	start_pfn = PAGE_ALIGN((unsigned long) &_end);

	/* locate the memory bank we're in
	 */

	base_pfn = start_img_pfn;

	for (i = 0; sp_banks[i].num_bytes != 0; i++) {

		/* do you feel luck, punk? */
		BUG_ON(i > SPARC_PHYS_BANKS);

		if (start_pfn < sp_banks[i].base_addr)
			continue;

		end_pfn = sp_banks[i].base_addr + sp_banks[i].num_bytes;

		if (start_pfn < end_pfn) {
			if (start_img_pfn != sp_banks[i].base_addr) {
				pr_warn("BOOTMEM: image start (0x%lx) does not "
					"coincide with start of memory "
					"bank (0x%lx), using start of bank.\n",
					sp_banks[i].base_addr, start_img_pfn);

				base_pfn =  sp_banks[i].base_addr;
			}

			break;
		}

		end_pfn = 0UL;
	}

	BUG_ON(!end_pfn);	/* found no suitable memory, we're boned */

	/* Map what is not used for the kernel code to the virtual page start.
	 * Since we don't have a bootstrap process for remapping the kernel,
	 * for now, we will run the code from the 1:1 mapping of our physical
	 * base and move everything else into high memory.
	 */

	start_pfn = (unsigned long) __pa(start_pfn);
	//start_pfn = PHYS_PFN(start_pfn);

	end_pfn = (unsigned long) __pa(end_pfn);
	//end_pfn = PHYS_PFN(end_pfn);

	pr_info("BOOTMEM: start page frame at 0x%lx\n"
		"BOOTMEM:   end page frame at 0x%lx\n",
	       start_pfn, end_pfn);


	/* the initial page map is statically allocated, hence set NULL,
	 * we add the first pool map here, everything after has to be allocated
	 * on top of that
	 */

	pg_node = MEM_PAGE_NODE(node);
	node++;

	(*pg_node) = &mm_init_page_node;

	(*pg_node)->pool              = &mm_init_page_pool;
	(*pg_node)->pool->block_order = &mm_init_block_order[0];
	(*pg_node)->pool->alloc_order = &mm_init_alloc_order[0];
	(*pg_node)->pool->blk_free    = &mm_init_bitmap[0];


	mem_size = end_pfn - base_pfn;

	/* we are misconfigured */
	BUG_ON(mem_size  > (1UL << MM_BLOCK_ORDER_MAX));
	BUG_ON(PAGE_SIZE < (1UL << MM_BLOCK_ORDER_MIN));

	BUG_ON(page_map_init(mm_init_page_map, base_pfn, end_pfn, PAGE_SIZE));


	/* reserve all space up to the end of the image, so mapping starts
	 * from the first free page following that section */
	BUG_ON(!page_map_reserve_chunk(start_pfn - base_pfn));


	/* our image has been reserved, now set up the boot memory allocator */
	chunk_pool_init(&phys_mem_pool, STACK_ALIGN, &bootmem_alloc_internal,
			&bootmem_free_internal, &bootmem_get_alloc_size);


	/* now add the remaining memory banks */
	for (i = 0; sp_banks[i].num_bytes != 0; i++) {

		BUG_ON(i > SPARC_PHYS_BANKS);

		/* this one has already been added */
		if (base_pfn == sp_banks[i].base_addr)
			continue;

		pg_node = MEM_PAGE_NODE(node);
		node++;

		if(!pg_node) {
			pr_err("BOOTMEM: initial page map holds less nodes "
			       "than number of configured memory banks.\n");
			break;
		}

		/* let's assume we always have enough memory, because if we
		 * don't, there is a serious configuration problem anyways
		 *
		 * XXX this should be a function...
		 */

		(*pg_node)		      = (struct page_map_node *) bootmem_alloc(sizeof(struct page_map_node));
		(*pg_node)->pool	      = (struct mm_pool *)       bootmem_alloc(sizeof(struct mm_pool));

		bzero((*pg_node)->pool, sizeof(struct mm_pool));

		(*pg_node)->pool->block_order = (struct list_head *)     bootmem_alloc(sizeof(struct list_head) * (MM_BLOCK_ORDER_MAX + 1));
		(*pg_node)->pool->alloc_order = (unsigned char *)        bootmem_alloc(MM_INIT_NUM_BLOCKS);
		(*pg_node)->pool->blk_free    = (unsigned long *)        bootmem_alloc(MM_INIT_LEN_BITMAP * sizeof(unsigned long));

		ret = page_map_add(sp_banks[i].base_addr,
				sp_banks[i].base_addr + sp_banks[i].num_bytes,
				PAGE_SIZE);

		if (ret) {
			pr_emerg("BOOTMEM: cannot add page map node\n");
			BUG();
		}
	}
}
