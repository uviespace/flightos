#include <kernel/kernel.h>
#include <kernel/mm.h>
#include <kernel/page.h>
#include <chunk.h>
#include <errno.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define STACK_ALIGN 8

/* power of 2 sizes */
#define MM_BLOCK_ORDER_MAX	28
#define MM_BLOCK_ORDER_MIN	12
#define PAGE_SIZE		(1 << MM_BLOCK_ORDER_MIN)

#define INIT_PAGE_MAP_MAX_ENTRIES 10

#define MM_INIT_NUM_BLOCKS MM_NUM_BLOCKS_TRACKABLE(MM_BLOCK_ORDER_MAX,\
                                                   MM_BLOCK_ORDER_MIN)

#define MM_INIT_LEN_BITMAP MM_BITMAP_LEN(MM_BLOCK_ORDER_MAX,\
                                         MM_BLOCK_ORDER_MIN)


#define MEM_PAGE_MAP		(mm_init_page_map)
#define MEM_PAGE_NODE(x)	(((x) <= INIT_PAGE_MAP_MAX_ENTRIES) ? \
				 &MEM_PAGE_MAP[(x)] : NULL)


#define ALIGN_MASK(x, mask)    (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)            ALIGN_MASK(x, (typeof(x))(a) - 1)
#define PAGE_ALIGN(addr)	ALIGN(addr, PAGE_SIZE)


#define NUM_BANKS 1

static struct physical_mem_bank {
        unsigned long base_addr;
        unsigned long num_bytes;
} sp_banks[NUM_BANKS + 1];

/* the pool we use for our boot memory allocator */
static struct chunk_pool phys_mem_pool;



unsigned long mm_init_bitmap[MM_INIT_LEN_BITMAP];
unsigned char mm_init_alloc_order[MM_INIT_NUM_BLOCKS];
struct list_head mm_init_block_order[MM_BLOCK_ORDER_MAX + 1];

struct mm_pool  mm_init_page_pool;
struct page_map_node  mm_init_page_node;
struct page_map_node *mm_init_page_map[INIT_PAGE_MAP_MAX_ENTRIES + 1];

/**
 * @brief physical memory allocator wrapper
 * @param size the number of bytes to allocate
 *
 * @return a pointer to a buffer or NULL on error
 */

static void *alloc_internal(size_t size)
{
	return page_map_reserve_chunk(size);
}


/**
 * @brief physical memory free wrapper
 * @param addr the address to free
 */

static void free_internal(void *ptr)
{
	page_free(ptr);
}

/**
 * @brief finds the actual size of an allocated buffer
 * @param addr the address to look up
 *
 * @return the buffer size, or 0 if invalid address or not found
 */

static size_t phys_get_alloc_size(void *ptr)
{
	return page_map_get_chunk_size(ptr);
}


/**
 * @brief allocate a buffer from the physical memory allocator
 *
 * @param size the number of bytes to allocate
 *
 * @return a pointer to a buffer or NULL on error
 */

void *phys_alloc(size_t size)
{
	void *ptr;

	unsigned long flags;


	if (!size)
		return NULL;

	ptr = chunk_alloc(&phys_mem_pool, size);

	if (!ptr) {
		printf("BOOTMEM: allocator out of memory\n");
		exit(-1);
	}

	return ptr;
}


/**
 * @brief release a buffer to the physical memory allocator
 *
 * @param addr the address to release
 */

void phys_free(void *ptr)
{
	chunk_free(&phys_mem_pool, ptr);
}




/**
 * @brief allocate and add a page map node
 */

static void init_page_map_node(struct page_map_node **pg_node)
{
	struct mm_pool *pool;


	/* let's assume we always have enough memory, because if we
	 * don't, there is a serious configuration problem anyways
	 */

	(*pg_node) = (struct page_map_node *) phys_alloc(sizeof(struct page_map_node));

	(*pg_node)->pool = (struct mm_pool *)  phys_alloc(sizeof(struct mm_pool));
	pool = (*pg_node)->pool;

	memset(pool, 0, sizeof(struct mm_pool));

	pool->block_order = (struct list_head *) phys_alloc(sizeof(struct list_head) * (MM_BLOCK_ORDER_MAX + 1));
	pool->alloc_order = (unsigned char *)    phys_alloc(MM_INIT_NUM_BLOCKS);
	pool->blk_free    = (unsigned long *)    phys_alloc(MM_INIT_LEN_BITMAP * sizeof(unsigned long));
}


/**
 * @brief initialise the boot memory
 */

void init(void)
{
	int i;

	int ret;


	unsigned long base_pfn = 0UL;
	unsigned long end_pfn = 0UL;
	unsigned long mem_size;

	unsigned long node = 0;

	struct page_map_node **pg_node;


	/* set initial bank */
	base_pfn = sp_banks[0].base_addr;
	end_pfn  = sp_banks[0].base_addr + sp_banks[0].num_bytes;

	/* the initial page map is statically allocated
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

	/* our image has been reserved, now set up the boot memory allocator */
	chunk_pool_init(&phys_mem_pool, STACK_ALIGN, &alloc_internal,
			&free_internal, &phys_get_alloc_size);


	/* now add the remaining memory banks */
	for (i = 0; sp_banks[i].num_bytes != 0; i++) {

		/* this one has already been added */
		if (base_pfn == sp_banks[i].base_addr)
			continue;

		pg_node = MEM_PAGE_NODE(node);
		node++;

		if(!pg_node) {
			printf("MEM: initial page map holds less nodes "
			       "than number of configured memory banks.\n");
			break;
		}

		init_page_map_node(pg_node);

		ret = page_map_add(sp_banks[i].base_addr,
				   sp_banks[i].base_addr + sp_banks[i].num_bytes,
				   PAGE_SIZE);

		if (ret == -ENOMEM) {	/* we ignore -EINVAL */
			printf("MEM: cannot add page map node\n");
			BUG();
		}

	}
}

void machine_halt(void)
{
	exit(-1);
}


#define P 30000

void run_test(void)
{
	int i, j;
	static uint32_t *p[P];
	uint32_t len[P];
	int v;


	while (1) {

		for (i = 0; i < P; i++) {
			if (p[i])
				continue;

			p[i] = page_alloc();
			if (!p[i])
				break;

			for (j = 0; j < PAGE_SIZE / sizeof(uint32_t); j++)
				p[i][j] = 0xb19b00b5;
		}

		if (rand() % (2 * P) != 0) {

			for (i = 0; i < P; i++) {

				v = rand();
				if (v % 5 != 0)
					continue;

				page_free(p[i]);
				p[i] = NULL;
			}
		} else {
			for (i = 0; i < P; i++) {
				page_free(p[i]);
				p[i] = NULL;
			}
		}
	}
}


int main(void)
{
	void *bank0;

	sp_banks[0].num_bytes = 128 * 1024 * 1024;
	bank0 = malloc(sp_banks[0].num_bytes + 4 * PAGE_SIZE);
	if (!bank0)
		exit(-1);

	sp_banks[0].base_addr = PAGE_ALIGN(((uint32_t) bank0));




	init();

	run_test();

	free(bank0);
}
