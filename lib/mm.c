/**
 *
 * @file lib/mm.c
 *
 * @ingroup buddy_mm
 * @defgroup buddy_mm Buddy-System Memory Manager
 * @brief a buddy-system memory manager
 *
 * This is a buddy-system memory manager.
 *
 *
 * ## Usage
 *
 * To use this at startup, the block list and bitmap must be allocated in the
 * platform setup code, since we have no allocation mechanism prior to this one.
 * If the kernel code is to be moved around or the memory is to be remapped,
 * expanded or something the sort (i.e. a "proper" memory bootstrap), one can
 * first allocate the appropriate area, then copy the configuration and update
 * it accordingly, then migrate to a run time configuration. This is not
 * possible at the moment, but trivial to implement.
 *
 *
 * ### Hints
 *
 * Since free blocks are tracked by attaching themselves to a list of their
 * own order, and the list head is stored in the head of the free block,
 * the corresponding memory is overwritten. In order to allow the user to
 * include their own code in the mapping (say, the kernel image), the maximum
 * pool order (i.e. the  managed memory block) is not linked initially, but only
 * after the first call.
 *
 * This obviously has the limitation of being unable to cleanly (i.e. without
 * tampering with the memory) include an arbitrary section of memory other
 * than from the base of the managed segment. Since we start to hand out memory
 * blocks from the base, this wouldn't make much sense anyway and complicate
 * things a lot. If you need to remap some arbitrary range, reserve the whole
 * chunk from the base and manage it on your own.
 *
 * @todo not atomic
 *
 * @example mm_demo.c
 */


#include <kernel/mm.h>
#include <kernel/err.h>

#include <kernel/kernel.h>
#include <kernel/printk.h>
#include <kernel/bitmap.h>
#include <kernel/log2.h>
#include <kernel/sysctl.h>
#include <kernel/string.h>
#include <kernel/init.h>
#include <page.h>


#include <asm/irqflags.h>
#include <asm/spinlock.h>


static struct spinlock mm_lock;

static struct {
	unsigned long total_blocks;
	unsigned long used_blocks;
	unsigned long alloc_fail;
} __mm_stat;

#ifdef CONFIG_SYSCTL
#if (__sparc__)
#define UINT32_T_FORMAT		"%lu"
#else
#define UINT32_T_FORMAT		"%u"
#endif

static ssize_t mm_show(__attribute__((unused)) struct sysobj *sobj,
			 __attribute__((unused)) struct sobj_attribute *sattr,
			 char *buf)
{
	/* note: the minimum block size is always be identical to the
	 * page size, as the page map mananger uses this component to
	 * track the blocks
	 */
	if (!strcmp(sattr->name, "total_blocks"))
		return sprintf(buf, UINT32_T_FORMAT, __mm_stat.total_blocks);

	if (!strcmp(sattr->name, "used_blocks"))
		return sprintf(buf, UINT32_T_FORMAT, __mm_stat.used_blocks);

	if (!strcmp(sattr->name, "free_blocks"))
		return sprintf(buf, UINT32_T_FORMAT, __mm_stat.total_blocks - __mm_stat.used_blocks);

	if (!strcmp(sattr->name, "alloc_fail")) {
		int ret;

		/* alloc_fail is self-clearing on read */
		ret = sprintf(buf, UINT32_T_FORMAT, __mm_stat.alloc_fail);
		__mm_stat.alloc_fail = 0;

		return ret;
	}

	return 0;
}


__extension__
static struct sobj_attribute total_blocks_attr = __ATTR(total_blocks,
							mm_show,
							NULL);
__extension__
static struct sobj_attribute used_blocks_attr = __ATTR(used_blocks,
						       mm_show,
						       NULL);
__extension__
static struct sobj_attribute free_blocks_attr = __ATTR(free_blocks,
						       mm_show,
						       NULL);

__extension__
static struct sobj_attribute alloc_fail_attr = __ATTR(alloc_fail,
						      mm_show,
						      NULL);

__extension__
static struct sobj_attribute *mm_attributes[] = {&total_blocks_attr,
						 &used_blocks_attr,
						 &free_blocks_attr,
						 &alloc_fail_attr,
						 NULL};

#endif

/**
 * free blocks are linked to a block order
 * by this structure held at the start of the block
 */

struct mm_blk_lnk {
	struct list_head link;
};



/**
 * @brief check if block address is within pool
 *
 * @param mp a struct mm_pool
 * @param blk a struct mm_blk_lnk
 *
 * @return true or false
 */

static bool mm_blk_addr_valid(struct mm_pool *mp, struct mm_blk_lnk *blk)
{
	if ((unsigned long) blk  >= mp->base)
		if ((unsigned long) blk  < mp->base + (1UL << mp->max_order))
			return true;

	return false;

//	return ((unsigned long) blk - (1UL << mp->max_order) < mp->base);
}


/**
 * @brief get the block index from its address
 *
 * @param mp a struct mm_pool
 * @param blk a struct mm_blk_lnk
 *
 * @return block index
 */

static unsigned long mm_blk_idx(struct mm_pool *mp, struct mm_blk_lnk *blk)
{
	return (((unsigned long) blk - mp->base) >> mp->min_order);
}


/**
 * @brief mark a block as free in the block bitmap
 *
 * @param mp a struct mm_pool
 * @param blk a struct mm_blk_lnk
 *
 * @return 0 if allocated, set otherwise
 */

static void mm_mark_free(struct mm_pool *mp, struct mm_blk_lnk *blk)
{
	__set_bit(mm_blk_idx(mp, blk), mp->blk_free);
}


/**
 * @brief mark a block as allocated in the block bitmap
 *
 * @param mp a struct mm_pool
 * @param blk a struct mm_blk_lnk
 *
 * @return 0 if allocated, set otherwise
 */

static void mm_mark_alloc(struct mm_pool *mp, struct mm_blk_lnk *blk)
{
	__clear_bit(mm_blk_idx(mp, blk), mp->blk_free);
}


/**
 * @brief test if a block is unallocated
 *
 * @param mp a struct mm_pool
 * @param blk a struct mm_blk_lnk
 *
 * @return 0 if allocated, set otherwise
 */

static int mm_blk_free(struct mm_pool *mp, struct mm_blk_lnk *blk)
{
	return test_bit(mm_blk_idx(mp, blk), mp->blk_free);
}

/**
 * @brief get allocation size of a block
 *
 * @param mp a struct mm_pool
 * @param blk a struct mm_blk_lnk
 *
 * @return the allocation size
 */

static unsigned long mm_blk_get_alloc_order(struct mm_pool *mp,
					    struct mm_blk_lnk *blk)
{
	return (unsigned long) mp->alloc_order[mm_blk_idx(mp, blk)];
}


/**
 * @brief set allocation size of a block
 *
 * @param mp a struct mm_pool
 * @param blk a struct mm_blk_lnk
 * @param size the allocation size
 *
 */

static void mm_blk_set_alloc_order(struct mm_pool *mp,
				  struct mm_blk_lnk *blk, unsigned long order)
{
	mp->alloc_order[mm_blk_idx(mp, blk)] =
			(typeof(mp->alloc_order[0])) order;
}


/**
 * @brief split a block to a new 2^n byte boundary
 *
 * @param mp  a struct mm_pool
 * @param blk a struct mm_blk_lnk
 * @param order the order of the block to split
 */

static void mm_split_blk(struct mm_pool *mp,
			 struct mm_blk_lnk *blk, unsigned long order)
{
	/* next block entry is 2^n bytes off */
	blk = (struct mm_blk_lnk *) ((unsigned long) blk + (1UL << order));

	mm_blk_set_alloc_order(mp, blk, order);

	mm_mark_free(mp, blk);

	list_add(&blk->link, &mp->block_order[order]);
}


/**
 * @brief locate a neighbouring block of the same order
 *
 * @param mp  a struct mm_pool
 * @param blk a struct mm_blk_lnk
 * @param order the order of the neighbouring block to locate
 *
 * @return the address pointer of the neighbour
 */

static void *mm_find_neighbour(struct mm_pool *mp,
			       struct mm_blk_lnk *blk, unsigned long order)
{
	unsigned long addr;


	addr = (unsigned long) blk - mp->base;

	addr ^= (1UL << order);
	addr += mp->base;


	return (void *) addr;
}


/**
 * @brief merge a block with a neighbouring block
 *
 * @param
 *
 * @return the address of the merged next-order block
 */

static struct mm_blk_lnk *mm_merge_blk(struct mm_pool *mp,
				   struct mm_blk_lnk *blk, unsigned long order)
{
	struct mm_blk_lnk *n, *t;


	INIT_LIST_HEAD(&blk->link);

	n = mm_find_neighbour(mp, blk, order);

	if (!mm_blk_free(mp, n))
		return NULL;

	if (mm_blk_get_alloc_order(mp, n) != order)
		return NULL;

	/* unlink and merge; the lower block address
	 * is the start of the newly created higher order block
	 */

	/* There is a potential bug, we should never get this far if the block
	 * was not allocated, even if the block address in free() was incorrect
	 */
	if (!n->link.prev || !n->link.next) {
		pr_crit("MM: corruption warning, someone tried to release an "
			"invalid block.\n");
		return NULL;
	}

	list_del(&n->link);

	if (n < blk) {
		t   = blk;
		blk = n;
		n   = blk;
	} else {

	}

	mm_blk_set_alloc_order(mp, n, 0);
	mm_blk_set_alloc_order(mp, blk, order + 1);

	/* cleanup is not explicitly needed, but makes bitmap dump look nicer */
	mm_mark_alloc(mp, n);
	mm_mark_alloc(mp, blk);


	return blk;
}


/**
 * @brief coalesce free neighbouring and higher order blocks
 *
 * @param mp    a struct mm_pool
 * @param blk	a struct mm_blk_lnk
 * @param order the order of the block
 */

static void mm_upmerge_blks(struct mm_pool *mp, struct mm_blk_lnk *blk)
{
	struct mm_blk_lnk *tmp;

	unsigned long order;


	order = mm_blk_get_alloc_order(mp, blk);

	while (order < mp->max_order) {
		tmp = mm_merge_blk(mp, blk, order);

		if (tmp)
			blk = tmp;
		else
			break;

		order++;
	}

	mm_mark_free(mp, blk);
	mm_blk_set_alloc_order(mp, blk, order);

	/* never link the initial block */
	if ((unsigned long) blk != mp->base)
		list_add(&blk->link, &mp->block_order[order]);
}


/**
 * @brief validate and fixup parameters if possible
 *
 * @param mp	a struct mm_pool
 * @param addr	an address pointer
 * @param order	a block order
 *
 * @return -EINVAL on error, (fixed) block order otherwise
 *
 * @note the caller address references the caller of mm_free() and mm_alloc()
 * @note an address of NULL is a valid argument to this call
 */

static unsigned long mm_fixup_validate(struct mm_pool *mp,
				       const void *addr, unsigned long order)
{
__diag_push();
__diag_ignore(GCC, 7, "-Wframe-address", "__builtin_return_address is just called for informative purposes here");
	if (!mp) {
		pr_info("MM: invalid memory pool specified in call from %p.\n",
			__caller(1));
		return -EINVAL;
	}

	if (order > mp->max_order) {
		pr_info("MM: requested order (%d) larger than maximum pool"
			"order (%d) in call from %p.\n",
			order, mp->max_order, __caller(1));
		return -EINVAL;
	}

	if (addr) {
		if ((unsigned long) addr < mp->base) {
			pr_info("MM: supplied address (%p) below base address "
				"of memory pool (0x%lx) in call from %p.\n",
				addr, mp->base, __caller(1));
			return -EINVAL;
		}

		if (((unsigned long) addr - mp->base)
		    > (1UL << mp->max_order)) {
			pr_info("MM: supplied address (%p) exceeds size "
				"of memory pool (%d bytes) in call from %p.\n",
				addr, (1UL << mp->max_order), __caller(1));
			return -EINVAL;
		}
	}

	if (order < mp->min_order) {
		pr_info("MM: requested order (%d) smaller than minimum pool"
			"order (%d) in call from %p, readjusting\n",
			order, mp->min_order, __caller(1));
		order = mp->min_order;
	}

__diag_pop(); /* -Wframe-address */

	return order;
}


/**
 * @brief allocate a block of memory
 *
 * @param mp a struct mm_pool
 * @param size the size to allocate
 *
 *
 * @return success: pointer to the start of the allocated memory block,
 *	   failure: NULL
 *
 * @note the allocated block will really be the next order 2^n the requested
 *	 size fits
 */

void *mm_alloc(struct mm_pool *mp, size_t size)
{
	unsigned long i;
	unsigned long order;

	struct mm_blk_lnk *blk = NULL;
	struct list_head *list = NULL;

	unsigned int flags;


	if (!mp)
		return NULL;

	if (!size)
		return NULL;

	order = ilog2(roundup_pow_of_two(size));

	pr_debug("MM: %lu bytes requested from allocator, block order is %lu\n",
		 size, order);

	order = mm_fixup_validate(mp, NULL, order);

	if (IS_ERR_VALUE(order))
	       return NULL;


	/* allocate first fit, by locating the first free block of the lowest
	 * possible order and dividing it down if necessary, i.e. to grab a
	 * block of order [1], with all order [1] blocks in use, a block of
	 * order [2] is requested and split in two. If no order [2] block is
	 * available, an order [3] block is requested, and split in two. Since
	 * only one order [2] blocks is needed to create 2 order [1] blocks,
	 * the second order [2] block remains available in the order [2] level.
	 *      ___________ ___________ ___________
	 * [3] |xxxxxxxxxxx|___________|___________| (...)
	 *                       v
	 *      ___________ ___________
	 * [2] |xxxxx|_____|_____|_____| (order [2] growth ->)
	 *              v     v
	 *      ___________ ______
	 * [1] |xx|xx|__|__|__|__| (order [1] growth ->)
	 *
	 * note: if the number of blocks allocate is zero, we start splitting
	 * blocks from our memory block base and the maximum block order
	 */


	flags = arch_local_irq_save();
	spin_lock_raw(&mm_lock);

	if (likely(mp->alloc_blks)) {

		for (i = order; i <= mp->max_order; i++) {
			list = &mp->block_order[i];

			if (!list_empty(list))
				break;
		}

		if(list_empty(list)) {
			__mm_stat.alloc_fail = 1;
			pr_debug("MM: pool %p out of blocks for order %lu\n",
				 mp, order);
			goto exit;
		}

		blk = list_entry(list->next, struct mm_blk_lnk, link);

		list_del(&blk->link);

	} else {

		blk = (struct mm_blk_lnk *) mp->base;
		i = mp->max_order;
	}

	mm_mark_alloc(mp, blk);
	mm_blk_set_alloc_order(mp, blk, order);

	/* re-link higher block order levels */
	while (order < i--)
		mm_split_blk(mp, blk, i);


	mp->alloc_blks += (1UL << (order - mp->min_order));

	__mm_stat.used_blocks += (1UL << (order - mp->min_order));

exit:

	spin_unlock(&mm_lock);
	arch_local_irq_restore(flags);
	return blk;
}


/**
 * @brief free a block of memory
 *
 * @param mp    a struct mm_pool
 * @param addr  the address of the block
 * @param order the order of the block
 *
 */

void mm_free(struct mm_pool *mp, const void *addr)
{
	unsigned long order;


	/* free() on NULL is fine */
	if (!addr)
		goto exit;

	if (!mm_blk_addr_valid(mp, (struct mm_blk_lnk *) addr))
		goto error;

	if (mm_blk_free(mp, (struct mm_blk_lnk *) addr))
		goto error;

	order = mm_blk_get_alloc_order(mp, (struct mm_blk_lnk *) addr);

	if (!order)
		goto error;

	order = mm_fixup_validate(mp, addr, order);

	if (!IS_ERR_VALUE(order)) {
		mm_upmerge_blks(mp, (struct mm_blk_lnk *) addr);
		mp->alloc_blks -= (1UL << (order - mp->min_order));

		__mm_stat.used_blocks -= (1UL << (order - mp->min_order));

		goto exit;
	}

error:
	pr_info("MM: double free, invalid size or untracked block %p in call "
	       "from %p\n", addr, __caller(0));

exit:
	return;
}



/**
 * @brief returns the size of the block for a given address
 *
 * @param mp	a struct mm_pool
 *
 * @param addr  the address of the block
 *
 * @return the size of the block the address is in, 0 if invalid or not found
 *
 */

unsigned long mm_block_size(struct mm_pool *mp, const void *addr)
{
	unsigned long order;

	unsigned long size = 0;


	if (mm_addr_in_pool(mp, (struct mm_blk_link *) addr)) {
		order = mm_blk_get_alloc_order(mp, (struct mm_blk_lnk *) addr);
		size = 1 << order;
	}

	return size;
}


/**
 * @brief returns number of free blocks at block granularity
 *
 * @param mp	a struct mm_pool
 *
 * @return number of free blocks at block granularity
 */

unsigned long mm_unallocated_blocks(struct mm_pool *mp)
{
	return mp->n_blks - mp->alloc_blks;
}


/**
 * @brief returns number of total free bytes tracked
 *
 * @return total number of free bytes
 */

size_t mm_free_bytes(void)
{
	return (__mm_stat.total_blocks - __mm_stat.used_blocks) * PAGE_SIZE;
}


/**
 * @brief returns number of allocated blocks at block granularity
 *
 * @param mp	a struct mm_pool
 *
 * @return number of allocated blocks at block granularity
 */

unsigned long mm_allocated_blocks(struct mm_pool *mp)
{
	return mp->alloc_blks;
}


/**
 * @brief check if block address is within pool
 *
 * @param mp	a struct mm_pool
 * @param addr	an address pointer
 *
 * @return true or false
 */

bool mm_addr_in_pool(struct mm_pool *mp, void *addr)
{
	if (mm_blk_idx(mp, (struct mm_blk_lnk *) addr) > mp->n_blks)
		goto nope;

	if (!mm_blk_addr_valid(mp, (struct mm_blk_lnk *) addr))
		goto nope;

	if (!mm_blk_get_alloc_order(mp, (struct mm_blk_lnk *) addr))
		goto nope;

	return true;

nope:
	return false;
}


/**
 *
 * @brief initialise the memory allocator instance
 *
 * @param base the base memory address of the managed memory pool
 * @param pool_size the size of the memory pool
 * @param granularity the block granularity
 *
 * @return 0 on success, -ENOMEM on error
 *
 * @note mp must come with preconfigured storage pointers
 *
 * @note If the pool sizes is not a power of two, it is rounded down to
 *	 the 2^n boundary. If the block size is not a power of two, it is
 *	 rounded up to the next 2^n boundary.
 */

int mm_init(struct mm_pool *mp, void *base,
	    size_t pool_size, size_t granularity)
{
	unsigned long i;


	mp->base  = (typeof(mp->base)) base;

	mp->max_order = ilog2(rounddown_pow_of_two(pool_size));
	mp->min_order  = ilog2(roundup_pow_of_two(granularity));

	if ((1UL << mp->max_order) != pool_size) {
		pr_notice("MM: pool size of %ld bytes is not a power of two, "
			  "adjusted to %ld.\n",
			  pool_size, (1UL << mp->max_order));
	}

	if ((1UL << mp->min_order) != granularity) {
		pr_notice("MM: block granularity of %ld bytes is not a power "
			  "of two, adjusted to %ld.\n",
			  granularity, (1UL << mp->min_order));
	}

	/* the smallest block unit must still fit the block link header */
	if ((1UL << mp->min_order) < sizeof(struct mm_blk_lnk)) {
		pr_err("MM: minimum allocatable block order of 2^%d "
		       "(%d bytes) is smaller than the required space for "
		       "the block header structure (%d bytes).\n",
		       mp->min_order, (1UL << mp->min_order),
		       sizeof(struct mm_blk_lnk));

		mp->min_order = ilog2(roundup_pow_of_two(
					sizeof(struct mm_blk_lnk)));

		pr_notice("MM: minimum allocatable block order adjusted "
			  "to 2^%d (%d bytes).\n",
			  mp->min_order, (1UL << mp->min_order));
	}

	/* The minimum block order must be smaller than the order of the pool.
	 * If we can't adjust the minimum order because of the condition above,
	 * we have to bail.
	 */
	if (mp->min_order > mp->max_order) {

		pr_err("MM: minimum allocatable block order (2^%d) is "
		       "greater than the order of the memory pool (2^%d).\n",
		       mp->min_order, mp->max_order);

		mp->min_order = mp->max_order - 1;

		if ((1UL << mp->min_order) < sizeof(struct mm_blk_lnk)) {
			pr_crit("MM: adjusted minimum block order (2^%d) does "
				"not fit the memory pool size, cannot "
				"continue.\n", mp->min_order);

			return -ENOMEM;
		}

		pr_notice("MM: minimum allocatable block order adjusted to "
			  "2^%d\n", mp->min_order);
	}

	for (i = 0; i <= mp->max_order; i++)
		INIT_LIST_HEAD(&mp->block_order[i]);

	mp->n_blks = MM_NUM_BLOCKS_TRACKABLE(mp->max_order, mp->min_order);

	__mm_stat.total_blocks += mp->n_blks;

	pr_info("MM: tracking %d blocks of %d bytes from base address %lx.\n",
		mp->n_blks, (1UL << mp->min_order), mp->base);

	/* initialise bitmap */
	bitmap_zero(mp->blk_free, mp->n_blks);

	/* make sure the allocated size is zeroed */
	bzero(mp->alloc_order, mp->n_blks * sizeof(typeof(mp->alloc_order[0])));

	/* fake initial allocation */
	mp->alloc_order[0] = (typeof(mp->alloc_order[0])) mp->max_order;

	mp->alloc_blks = mp->n_blks;

	__mm_stat.used_blocks += mp->alloc_blks;

	/* we start by dividing the highest order block, mark it as available */
	mm_free(mp, base);

	return 0;
}

#ifdef CONFIG_SYSCTL
/**
 * @brief initialise the sysctl entries for the
 *
 * @return -1 on error, 0 otherwise
 *
 * @note we set this up as a late initcall since we need sysctl to be
 *	 configured first
 */

static int mm_init_sysctl(void)
{
	struct sysobj *sobj;


	sobj = sysobj_create();

	if (!sobj)
		return -1;

	sobj->sattr = mm_attributes;

	sysobj_add(sobj, NULL, sysctl_root(), "mm");

	return 0;
}
late_initcall(mm_init_sysctl);
#endif /* CONFIG_SYSCTL */


/**
 * @brief cleanup function
 *
 * @param mp a struct mp_pool
 *
 * @note does nothing at this time
 */

void mm_exit(struct mm_pool *mp) {
	(void) mp;
}


/**
 * @brief dump pool statistics
 *
 * @param mp a struct mm_pool
 */

void mm_dump_stats(struct mm_pool *mp)
{
	unsigned long i __attribute__((unused));


	if (!mp)
		return;

#ifdef CONFIG_MM_DEBUG_DUMP
	printk("\n\n"
	       "MM: MEMORY POOL DUMP\n"
	       "MM: base %p, size %ld bytes (order %ld) "
		   "block granularity %ld bytes (order %ld)\n"
	       "MM: \n"
	       "MM: block usage bitmap:\n",
	       mp->base,
	       (1UL << mp->max_order), mp->max_order,
	       (1UL << mp->min_order),  mp->min_order);

#ifdef CONFIG_MM_DEBUG_DUMP_ALLOC_BITMAP
	bitmap_print(mp->blk_free, mp->n_blks);
#endif /* CONFIG_MM_DEBUG_DUMP_ALLOC_BITMAP */

#ifdef CONFIG_MM_DEBUG_DUMP_BLOCK_ALLLOC
	printk("\n"
	       "MM: BLOCK ALLOCATION ORDERS\n");
	for (i = 0; i < mp->n_blks; i++) {
		if (mp->alloc_order[i])
		printk("[%2d]", mp->alloc_order[i]);
		else
		printk("[  ]");
		if (i && ((i+1) % 32) == 0)
			printk("\n");
	}
	printk("\n");
#endif /* CONFIG_MM_DEBUG_DUMP_BLOCK_ALLLOC */

#ifdef CONFIG_MM_DEBUG_DUMP_BLOCK_STATS
	printk("\n"
	       "MM: BLOCK STATS\n");

	for (i = mp->min_order; i <= mp->max_order; i++) {

		unsigned long cnt = 0;
		struct list_head *entry;

		list_for_each(entry, &mp->block_order[i])
			cnt++;

		printk("MM: order [%2lu]: %lu free block(s)\n", i, cnt);

	}
#endif /* CONFIG_MM_DEBUG_DUMP_BLOCK_STATS */

	printk("MM:: END OF DUMP\n\n");

#endif /* CONFIG_MM_DEBUG_DUMP */
}
