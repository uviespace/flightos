#include <stdio.h>

#include <chunk.h>
#include <stdlib.h>



/* configure a dummy higher-tier memory-manager */

#define PAGE_SIZE 4096

static void *page_alloc(size_t size)
{
	return malloc(PAGE_SIZE);
}

static void page_free(void *addr)
{
	free(addr);
}

static size_t get_alloc_size()
{
	return PAGE_SIZE;
}


#define DO_ALLOC	256
#define ALIGN_BYTES	8

int main(void)
{
	int i;

	struct chunk_pool pool;

	void *p[DO_ALLOC];


	chunk_pool_init(&pool, ALIGN_BYTES,
			&page_alloc, &page_free, &get_alloc_size);

	for (i = 0; i < DO_ALLOC; i++)
		p[i] = chunk_alloc(&pool, (i + 1) * 17);

	for (i = DO_ALLOC - 1; i >= 0; i--)
		chunk_free(&pool, p[i]);
}
