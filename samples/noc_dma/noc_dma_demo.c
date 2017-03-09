#include <kernel/kmem.h>
#include <kernel/kernel.h>
#include <kernel/printk.h>
#include <noc_dma.h>
#include <noc.h>

static void noc_dma_2d_print(unsigned long *p, size_t x_sz, size_t y_sz)
{
	size_t x;
	size_t y;


	for (y = 0; y < y_sz; y++) {
		for (x = 0; x < x_sz; x++) {
			if (p[x + y * x_sz])
				printk("%02x ", p[x + y * x_sz]);
			else
				printk(" . ", p[x + y * x_sz]);
		}
		printk("\n");
	}
}


/**
 * @brief execute transfer types 1-4 and reproduce equivalent figure
 * @see MPPB datasheet v4.03, p62, Tbl. 73
 */

void noc_dma_test_transfers(void)
{
	int i;

	unsigned long *src;
	unsigned long *dst;


	const uint16_t x_size = 16;
	const uint16_t y_size = 19;


	src = kcalloc(x_size * y_size, sizeof(unsigned long));
	BUG_ON(!src);

	dst = (unsigned long *) NOC_SCRATCH_BUFFER_BASE;
	BUG_ON(!dst);

	/* initialise */
	for (i = 0; i < (x_size * y_size); i++) {
		src[i] = i + 1;
		dst[i] = 0;
	}
	/* base */
	BUG_ON(noc_dma_req_lin_xfer(src, (char *) dst + 0x0, 32, WORD, LOW, 256,
				    NULL, NULL));
	/* type 1 */
	BUG_ON(noc_dma_req_xfer(src, (char *) dst + 0x100, 16, 1, WORD,
				2, 1, 0, 0, LOW, 256,
				NULL, NULL));

	/* type 2 */
	BUG_ON(noc_dma_req_xfer(src, (char *) dst + 0x1C0, 6, 2, WORD,
				1, 1, 16, 16, LOW, 256,
				NULL, NULL));

	/* type 3 */
	BUG_ON(noc_dma_req_xfer(src, (char *) dst + 0x2C0, 4, 2, WORD,
				1, 16, 4, 1, LOW, 256,
				NULL, NULL));
	/* type 4 */
	BUG_ON(noc_dma_req_xfer(src, (char *) dst + 0x44C, 4, 4, WORD,
				1, -1, 4, 4, LOW, 256,
				NULL, NULL));
	printk("\n--- {DST} ---\n");
	noc_dma_2d_print(dst, x_size, y_size);

	kfree(src);
}

