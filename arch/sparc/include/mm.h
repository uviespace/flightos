/**
 * @file arch/sparc/include/mm.h
 */

#ifndef _SPARC_MM_H_
#define _SPARC_MM_H_

#include <kernel/mm.h>
#include <kernel/bootmem.h>
#include <kernel/page.h>

/* The following structure is used to hold the physical
 * memory configuration of the machine.  This is filled in
 * prom_meminit() and is later used by mem_init() to set up
 * mem_map[].  We statically allocate SPARC_PHYS_BANKS+1 of
 * these structs, this is arbitrary.  The entry after the
 * last valid one has num_bytes==0.
 */

struct sparc_physical_banks {
	unsigned long base_addr;
	unsigned long num_bytes;
};

#if defined (CONFIG_EXTRA_SPARC_PHYS_BANKS)
#define SPARC_PHYS_BANKS CONFIG_EXTRA_SPARC_PHYS_BANKS
#else
#define SPARC_PHYS_BANKS 0
#endif

extern struct sparc_physical_banks sp_banks[SPARC_PHYS_BANKS + 1];


/* linker symbol marking the the start of the program (image) */
extern char _start[];
/* linker symbol marking the the end of the program */
extern char _end[];


/* The default configuration allows for at most 32 MiB sized blocks
 * with a minimum block size of 4 kiB, requiring a bit map of 16 kiB to track
 * all blocks.
 */

#if defined(CONFIG_SPARC_MM_BLOCK_ORDER_MAX)
#define MM_BLOCK_ORDER_MAX CONFIG_SPARC_MM_BLOCK_ORDER_MAX
#else
#define MM_BLOCK_ORDER_MAX 26
#endif

#if defined(CONFIG_SPARC_MM_BLOCK_ORDER_MIN)
#define MM_BLOCK_ORDER_MIN CONFIG_SPARC_MM_BLOCK_ORDER_MIN
#else
#define MM_BLOCK_ORDER_MIN 12 /* SRMMU page size */
#endif

compile_time_assert(MM_BLOCK_ORDER_MIN < MM_BLOCK_ORDER_MAX,\
		    MM_BLOCK_LIMITS_INVALID);


#define MM_INIT_NUM_BLOCKS MM_NUM_BLOCKS_TRACKABLE(MM_BLOCK_ORDER_MAX,\
						   MM_BLOCK_ORDER_MIN)

#define MM_INIT_LEN_BITMAP MM_BITMAP_LEN(MM_BLOCK_ORDER_MAX,\
					 MM_BLOCK_ORDER_MIN)

/* initial block tracking bitmap */
extern unsigned long mm_init_bitmap[MM_INIT_LEN_BITMAP];
/* initial block allocation size tracking */
extern unsigned char mm_init_alloc_order[MM_INIT_NUM_BLOCKS];
/* initial block order anchor */
extern struct list_head mm_init_block_order[MM_BLOCK_ORDER_MAX + 1];



#define BOOTMEM_CHUNKSIZE (1 << CONFIG_SPARC_BOOTMEM_RESERVE_CHUNK_ORDER)



#endif /*_SPARC_MM_H_ */
