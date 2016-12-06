/**
 * @file arch/sparc/kernel/page.c
 */

#include <page.h>

struct pg_data page_mem[SPARC_PHYS_BANKS+1];



unsigned long init_page_map(struct pg_data *pg,
			    unsigned long start_pfn,
			    unsigned long end_pfn)
{
	unsigned long mapsize = 0;
#if 0
	pg->bdata->node_map_mem = __va(PFN_PHYS(start_pfn));
	pg->bdata->node_mem_map = __va(PFN_PHYS(start_pfn));
	pg->bdata->node_min_pfn = start_pfn;
	pg->bdata->node_low_pfn = end_pfn;
	link_bootmem(pg->bdata);

	/*
	 * Initially all pages are reserved - setup_arch() has to
	 * register free RAM areas explicitly.
	 */
	mapsize = bootmap_bytes(end - start);
	memset(bdata->node_bootmem_map, 0xff, mapsize);

	bdebug("nid=%td start=%lx map=%lx end=%lx mapsize=%lx\n",
		bdata - bootmem_node_data, start, mapstart, end, mapsize);

#endif
	return mapsize;
}

