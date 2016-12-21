/**
 * @file arch/sparc/kernel/page.c
 */

#include <page.h>



/* things we need statically allocated in the image (i.e. in .bss)
 * at boot
 */

unsigned long mm_init_bitmap[MM_INIT_LEN_BITMAP];
unsigned char mm_init_alloc_order[MM_INIT_NUM_BLOCKS];
struct list_head mm_init_block_order[MM_BLOCK_ORDER_MAX + 1];

struct mm_pool  mm_init_page_pool;
struct page_map_node  mm_init_page_node;
struct page_map_node *mm_init_page_map[INIT_PAGE_MAP_MAX_ENTRIES + 1];

