/**
 * @file arch/sparc/kernel/bootmem.c
 */

#include <page.h>

#include <kernel/printk.h>


void bootmem_init(void)
{
	int i;

	unsigned long start_pfn;
	unsigned long end_pfn = 0UL;


	pr_notice("End of program at: %lx\n", (unsigned long) _end);

	/* start allocatable memory with page aligned address of last symbol in
	 * image
	 */
	start_pfn  = (unsigned long) PAGE_ALIGN((unsigned long) &_end);

	/* locate the memory bank we're in and start the mapping from
	 * the first free page after the image.
	 */
	for (i = 0; sp_banks[i].num_bytes != 0; i++) {

		if (start_pfn < sp_banks[i].base_addr)
			continue;

		end_pfn = sp_banks[i].base_addr + sp_banks[i].num_bytes;

		if (start_pfn < end_pfn)
			break;
		
		end_pfn = 0UL;
	}

	BUG_ON(!end_pfn);	/* found no suitable memory, we're boned */

	/* Map what is not used for the kernel code to the virtual page start.
	 * Since we don't have a bootstrap process for remapping the kernel,
	 * for now, we will run the code from the 1:1 mapping of our physical
	 * base and move everything else into high memory.
	 */

	start_pfn = (unsigned long) __pa(start_pfn);
	
	/* Now shift down to get the real physical page frame number. */
	start_pfn >>= PAGE_SHIFT;
	
	end_pfn = (unsigned long) __pa(end_pfn);

	end_pfn = end_pfn >> PAGE_SHIFT;
	
	pr_notice("start_pfn: %lx\n", start_pfn);
	pr_notice("end_pfn:   %lx\n", end_pfn);


	init_page_map(MEM_PAGE_NODE(0), start_pfn, end_pfn);





}
