/**
 * @file arch/sparc/kernel/bootmem.c
 */

#include <page.h>

#include <string.h>

#include <kernel/printk.h>
#include <kernel/kernel.h>


/* TODO still demo code */
void bootmem_init(void)
{
	int i;

	unsigned long base_pfn;
	unsigned long start_pfn;
	unsigned long start_img_pfn;
	unsigned long end_pfn = 0UL;
	unsigned long mem_size;

	void *pages[2048];

	int t=0;

	struct page_map_node **pg_node;


	pr_info("BOOTMEM: start of program image at %p\n", start);
	pr_info("BOOTMEM:   end of program image at %p\n", end);


	/* lowest page frame number coincides with page aligned address
	 * start symbol in image, which hopefully coincides with the start
	 * of the RAM we are running from.
	 */
	start_img_pfn  = (unsigned long) PAGE_ALIGN((unsigned long) &start);

	/* start allocatable memory with page aligned address of last symbol in
	 * image, everything before will be reserved
	 */
	start_pfn  = (unsigned long) PAGE_ALIGN((unsigned long) &end);

	/* locate the memory bank we're in
	 */

	base_pfn = start_img_pfn;

	for (i = 0; sp_banks[i].num_bytes != 0; i++) {

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

	pr_info("BOOTMEM: start page frame number: 0x%lx\n"
		"BOOTMEM:   end page frame number: 0x%lx\n",
	       start_pfn, end_pfn);


	/* the initial page map is statically allocated, hence set NULL,
	 * we add the first pool map here, everything after has to be allocated
	 * on top of that
	 */

	pg_node = MEM_PAGE_NODE(0);

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


	mm_dump_stats((*pg_node)->pool);

	page_alloc();
	page_alloc();
	page_alloc();
	page_alloc();
	page_alloc();

	mm_dump_stats((*pg_node)->pool);



	pg_node = MEM_PAGE_NODE(1);

	BUG_ON(!pg_node);

	(*pg_node)		      = page_map_reserve_chunk(
					sizeof(struct page_map_node));

	(*pg_node)->pool	      = page_map_reserve_chunk(
					sizeof(struct mm_pool));

	bzero((*pg_node)->pool, sizeof(struct mm_pool));

	(*pg_node)->pool->block_order = page_map_reserve_chunk(
					sizeof(struct list_head) *
					MM_BLOCK_ORDER_MAX);
	(*pg_node)->pool->alloc_order = page_map_reserve_chunk(
					MM_INIT_NUM_BLOCKS);
	(*pg_node)->pool->blk_free    = page_map_reserve_chunk(
					MM_INIT_LEN_BITMAP *
					sizeof(unsigned long));


	base_pfn = (unsigned long) page_map_reserve_chunk(1024*4*4);
	page_map_add(base_pfn, base_pfn + 1024*4*4, PAGE_SIZE);



	pg_node = MEM_PAGE_NODE(2);
	BUG_ON(!pg_node);

	(*pg_node)		      = page_map_reserve_chunk(
					sizeof(struct page_map_node));
	BUG_ON(!(*pg_node));

	(*pg_node)->pool	      = page_map_reserve_chunk(
					sizeof(struct mm_pool));

	bzero((*pg_node)->pool, sizeof(struct mm_pool));
	(*pg_node)->pool->block_order = page_map_reserve_chunk(
					sizeof(struct list_head) *
					MM_BLOCK_ORDER_MAX);
	(*pg_node)->pool->alloc_order = page_map_reserve_chunk(
					MM_INIT_NUM_BLOCKS);
	(*pg_node)->pool->blk_free    = page_map_reserve_chunk(
					MM_INIT_LEN_BITMAP *
					sizeof(unsigned long));


	base_pfn = (unsigned long) page_map_reserve_chunk(1024*4*4);
	page_map_add(base_pfn, base_pfn + 1024*4*4, PAGE_SIZE);


	while (t < 1740)
		pages[t++] = page_alloc();

	pages[t++] = page_alloc();
	pages[t++] = page_alloc();
	pages[t++] = page_alloc();
	pages[t++] = page_alloc();
	pages[t++] = page_alloc();
	pages[t++] = page_alloc();
	pages[t++] = page_alloc();
	pages[t++] = page_alloc();
	pages[t++] = page_alloc();

	/* NULL */
	pages[t++] = page_alloc();

	page_free(pages[--t]);
	page_free(pages[--t]);
	page_free(pages[--t]);
	page_free(pages[--t]);
	page_free(pages[--t]);
	page_free(pages[--t]);


}
