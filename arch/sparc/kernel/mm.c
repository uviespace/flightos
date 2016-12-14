/**
 * @file arch/sparc/kernel/mm.c
 */

#include <mm.h>
#include <mm.h>


unsigned long phys_base;
unsigned long pfn_base;


#define SPARC_MM_INIT_NUM_BLOCKS MM_NUM_BLOCKS_TRACKABLE(MM_BLOCK_ORDER_MAX,\
							 MM_BLOCK_ORDER_MIN)

#define SPARC_MM_INIT_LEN_BITMAP MM_BITMAP_LEN(MM_BLOCK_ORDER_MAX,\
					       MM_BLOCK_ORDER_MIN)


struct sparc_physical_banks sp_banks[SPARC_PHYS_BANKS+1];

unsigned long mm_init_bitmap[SPARC_MM_INIT_LEN_BITMAP];
unsigned char mm_init_alloc_order[SPARC_MM_INIT_NUM_BLOCKS];
struct list_head mm_init_block_order[MM_BLOCK_ORDER_MAX + 1];
