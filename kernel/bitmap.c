/**
 * @file kernel/bitmap.c
 */


#include <kernel/bitmap.h>

#include <kernel/printk.h>
#include <kernel/log2.h>


/**
 * @brief print a bitmap
 *
 * @param bitarr an array
 * @param nr the number of bits to print
 *
 * @note assumes big endian
 */

void bitmap_print(const unsigned long *bitarr, unsigned long nr)
{
	int i, j;

	unsigned long chunksz;


	for (i = 0; i < roundup_pow_of_two(nr / BITS_PER_LONG); i++ ) {

		/* for nicer looking output */
		if ((i % 2) == 0)
			printk("\n [%2ld] ", i);
		else
			printk(" ");


		if (nr > BITS_PER_LONG)
			chunksz = BITS_PER_LONG;
		else
			chunksz = nr;

		for (j = 0; j < chunksz; j++) {
			if ((bitarr[i] >> j) & 1)
				printk("|");
			else
				printk(".");
		}

		nr -= chunksz;
	}

	printk("\n");
}
