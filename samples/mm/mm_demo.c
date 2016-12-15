#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <kernel/mm.h>
#include <kernel/printk.h>
#include <kernel/bitmap.h>


/* pool size (2^n) and granularity (same)*/
#define MM_BLOCK_ORDER_MAX 17
#define MM_BLOCK_ORDER_MIN  8

/* block size to allocate (must be at least 2^MM_BLOCK_ORDER_MIN) */
#define ALLOC_BYTES  (1UL << MM_BLOCK_ORDER_MIN)
/* how many blocks to allocate in a loop */
#define TRY_BLOCKS 27



/* nothing to do here */
#define MM_NUM_BLOCKS MM_NUM_BLOCKS_TRACKABLE(MM_BLOCK_ORDER_MAX,\
					      MM_BLOCK_ORDER_MIN)

#define MM_LEN_BITMAP MM_BITMAP_LEN(MM_BLOCK_ORDER_MAX,\
				    MM_BLOCK_ORDER_MIN)

#define POOL_SIZE	  (1UL << MM_BLOCK_ORDER_MAX)
#define BLOCK_GRANULARITY (1UL << MM_BLOCK_ORDER_MIN)

unsigned char mm_init_alloc_order[MM_NUM_BLOCKS];
unsigned long mm_init_bitmap[MM_LEN_BITMAP];
struct list_head mm_init_block_order[MM_BLOCK_ORDER_MAX + 1];


int main(void)
{
	int j;

	void *pool;
	struct mm_pool mp;

	unsigned long i[TRY_BLOCKS];


       	pool = malloc(POOL_SIZE);

	if (!pool)
		return 0;

	/* mm_init() expects these to be all set */
	mp.block_order = &mm_init_block_order[0];
	mp.alloc_order = &mm_init_alloc_order[0];
	mp.blk_free    = &mm_init_bitmap[0];


	if (mm_init(&mp, pool, POOL_SIZE, BLOCK_GRANULARITY)) {
		printk("Error initialising the memory manager\n");
		return -1;
	}

	mm_dump_stats(&mp);

	printf("---{ALLOCATING BLOCKS}---\n");

	for (j = 0; j < TRY_BLOCKS; j++) {
		i[j] = (unsigned long) mm_alloc(&mp, ALLOC_BYTES);
		bitmap_print(mm_init_bitmap, mp.n_blks);
	}

	mm_dump_stats(&mp);


	printf("---{FREEING BLOCKS}---\n");

	for (j = (TRY_BLOCKS - 1); j >= 0; j--) {
		mm_free(&mp, (void *) i[j]);
		bitmap_print(mm_init_bitmap, mp.n_blks);
	}
	mm_dump_stats(&mp);


	printf("---{NULL FREE}---\n");
	printf("(valid, should give no response)\n");
	mm_free(&mp, NULL);


	printf("---{DOUBLE FREE}---\n");
	mm_free(&mp, (void *) i[0]);


	printf("---{INVALID FREE}---\n");
	mm_free(&mp, (void *) 0xb19b00b5);


	mm_exit(&mp);

	free(pool);

	return 0;
}
