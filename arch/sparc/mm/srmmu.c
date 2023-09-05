/**
 *
 * @file arch/sparc/mm/srmmu.c
 *
 * @ingroup srmmu
 * @defgroup srmmu SPARC Reference MMU
 *
 * @brief access to the SPARC Reference MMU (SRMMU)
 *
 *
 * ## Overview
 *
 * This implements SRMMU functionality
 *
 * ## Mode of Operation
 *
 * This is how the srmmu context table works:
 *
 * "level 0" is the context pointer that is used by the MMU to determine which
 * table to use, if a certain context is selected via srmmu_set_ctx()
 * Since level 0 corresponds to a 4 GiB page mapping, it is also possible to
 * use it as a page table entry, i.e. to transparently map the whole 32bit
 * address through the MMU, you can use the context table pointer (ctp) as
 * page table entry (pte):
 *
 * @code{.c}
 *	srmmu_set_ctx_tbl_addr((unsigned long) _mmu_ctp);
 *	_mmu_ctp[0] = SRMMU_PTE(0x0, (SRMMU_CACHEABLE | SRMMU_ACC_S_RWX_2));
 *	srmmu_set_ctx(0);
 * @endcode
 *
 * so the virtual address mapping starts at physical address address 0x0.
 * Note: accessing memory regions not actually mapped in hardware will
 * subsequently raise data access exceptions!
 *
 *
 * If you are only interested in protecting larger chunks of memory, i.e.
 * against accidential R/W/X operations, you can point a level 0 ctp to a
 * level 1 table by setting it up as a page table descriptor (ptd):
 *
 * @code{.c}
 *	_mmu_ctp[0] = SRMMU_PTD((unsigned long) &ctx->tbl.lvl1[0]);
 * @endcode
 *
 * Let's say you want to map a 16 MiB chunk starting at 0x40000000
 * transparently through the MMU, you would then set lvl 1 table entry
 * 0x40 (64), since lvl1 tables addresses are formed by the two highest-order
 * bytes:
 *
 * @code{.c}
 *	ctx->tbl.lvl1[0x40] =
 *		SRMMU_PTE(0x40000000, (SRMMU_CACHEABLE | SRMMU_ACC_S_RWX_2));
 * @endcode
 *
 * an then select the MMU context.
 *
 * From the above example, you can already guess how the MMU context tables
 * work. If you access address 0x40000000, the MMU will take the two highest
 * order bytes and use them as an offset into the level 1 table. If it finds
 * that the entry is marked as pte, it takes the corresponding two highest-order
 * address bytes in the entry and replaces the bytes in your address with those
 * bytes.
 *
 * Say you try to access 0x40123456, the MMU will strip 0x40 and look into the
 * table, where (in our case) finds the referenced physical address bytes to be
 * 0x40 as well, and will hence take your 6 lower order bytes 0x123456 and put
 * 0x40 in front again, resulting in the translated address 0x40123456.
 *
 * Let's say you created a second mapping to the same physical address:
 *
 * @code{.c}
 *	ctx->tbl.lvl1[0xaa] =
 *		SRMMU_PTE(0x40000000, (SRMMU_CACHEABLE | SRMMU_ACC_S_RWX_2));
 * @endcode
 *
 * and try to access 0xaa123456. The MMU will then replace 0xaa with 0x40, and
 * you'll get the same physical address again!
 *
 * Note: You could actually use this to map flat physical memory into a
 * virtual "circular" topology by repeating the mapping for a range of virtual
 * address space and use an automatically underflowing buffer index
 * (i.e. a 8 or 16 bit integer type).
 *
 * Now, if you wanted to map a 16 MiB chunk into smaller sections, rather than
 * setting up the entry as a pte, you would configure it as a ptd that
 * references a lvl 2 table, e.g.
 *
 * @code{.c}
 *	ctx->tbl.lvl1[0x40] = SRMMU_PTD((unsigned long) &ctx->tbl->lvl2[0]);
 * @endcode
 *
 * and then set up the lvl 2 table entries, which reference 64 chunks of 256 KiB
 * each, which means that the next 6 _bits_ in the address are used by the MMU
 * for the offset into the table, i.e. (addr & 0x00FC0000 ) >> 18
 *
 * If you configure a mapping for 0xaa04xxxx, you would set:
 *
 * @code{.c}
 *	ctx->tbl.lvl2[0xaa][0x01] =
 *		SRMMU_PTE(0x40000000, (SRMMU_CACHEABLE | SRMMU_ACC_S_RWX_2));
 * @endcode
 *
 * and then access 0xaa04cdef, the MMU will strip the upper 14 bits from that
 * address and replace them with the upper 14 bits of 0x40000000 and hence
 * reference a physical address of 0x4000cdef
 *
 * The same applies to level 3 contexts, which are 4 kiB chunks, so here the
 * MMU replaces the upper 20 _bits_ of your address.
 *
 *
 * @note Unused context table entries must be set accordingly, otherwise
 * the MMU might try to establish a mapping, so initialise all context table
 * entries to SRMMU_ENTRY_TYPE_INVALID.
 *
 *
 * ## Error Handling
 *
 * TODO
 *
 * ## Notes
 *
 * We only allow large (16 MiB) and small (4 kiB) page mappings. The intermediate
 * (256 kiB) level is always explcitly mapped via small pages.
 *
 * The level 1 context table is always allocated, level 2 and 3 tables are not
 * and are allocated/released as needed. The allocation is done via a low-level
 * allocator, that is specified via the srmmu_init_ctx() call and may be
 * different for each context in the context list. The alloc() call is expected
 * to either return a valid buffer of at least the size requested, or NULL on
 * error.
 *
 * If a mapping is released, all allocations are released by walking the tree,
 * since we don't track them separately. If it turns out that this is done
 * often, it might for performance reasons be prefereable to maintain SLABS of
 * page tables, i.e. one per mapping size, where we take and return them as
 * needed. We could also track them of course...
 *
 *
 * @todo this needs some cleanup/streamlining/simplification
 */


#include <kernel/kernel.h>
#include <kernel/printk.h>
#include <list.h>

#include <page.h>
#include <mm.h>
#include <srmmu.h>
#include <srmmu_access.h>
#include <errno.h>



static unsigned long *_mmu_ctp;	      /* table of mmu context table pointers */
static unsigned long  _mmu_num_ctp;   /* number of contexts in the ctp       */
static struct mmu_ctx *_mmu_ctx;      /* the currently configured context    */



/**
 * The magic entry is placed one word prior to the start of the aligned
 * address. If the allocation address coincides with the aligned address,
 * a magic marker is placed in the word following the end of the table.
 * since the offset is at most (SRMMU_TBL_LVL_1_ALIGN - 1), this is the
 * mask that is used for the _negative_ offset to the allocated address.
 *
 * @note we can remove this once we have chunk_alloc_aligned() (which will
 * subsequently require a function bootmem_alloc_aligned())
 *
 * @note the above needs another solution to detect empty subtables
 */

/* XXX: it works for now, but this really needs some improvement, at least
 *	a checkbit or so...
 */
#define MAGIC_MARKER (0xdeadda7a & ~(SRMMU_TBL_LVL_1_ALIGN - 1))
#define MAGIC(offset) (((offset) & (SRMMU_TBL_LVL_1_ALIGN - 1)) | MAGIC_MARKER)
#define OFFSET_FROM_MAGIC(x) ((x) & (SRMMU_TBL_LVL_1_ALIGN - 1))
#define IS_MAGIC(x) (((x) & ~(SRMMU_TBL_LVL_1_ALIGN - 1)) == MAGIC_MARKER)

#define MAGIC_REF_CNT(x) (x)
#define REF_FROM_MAGIC(x) (x)

#define MAGIC_WORDS 2

/**
 * table level lvl1 is always SRMMU_SIZE_TBL_LVL_1 entries
 * depending on the number of forward references (ptd's) into the
 * higher level tables, the following applies:
 *
 * lvl2: at most SRMMU_SIZE_TBL_LVL_1 * SRMMU_SIZE_TBL_LVL_2
 * lvl3: at most n_lvl2 * SRMMU_SIZE_TBL_LVL_3
 *
 */

struct mmu_ctx_tbl {
	struct srmmu_ptde *lvl1;
};

struct mmu_ctx {
	struct mmu_ctx_tbl tbl;

	unsigned int ctx_num;

	void *(*alloc)(size_t size);
	void  (*free) (void *addr);


	/* lower and upper boundary of unusable memory space */
	unsigned long lo_res;
	unsigned long hi_res;

	struct list_head node;
};


static struct list_head ctx_free;
static struct list_head ctx_used;





static inline void del_mmu_ctx_from_list(struct mmu_ctx *ctx)
{
	list_del(&ctx->node);
}

static inline void add_mmu_ctx_to_list(struct mmu_ctx *ctx,
				       struct list_head *list)
{
	list_add_tail(&ctx->node, list);
}

static inline void mmu_ctx_add_free(struct mmu_ctx *ctx)
{
	list_add_tail(&ctx->node, &ctx_free);
}

static inline void mmu_ctx_add_used(struct mmu_ctx *ctx)
{
	list_add_tail(&ctx->node, &ctx_used);
}

static inline void mmu_ctx_move_free(struct mmu_ctx *ctx)
{
	list_move_tail(&ctx->node, &ctx_free);
}

static inline void mmu_ctx_move_used(struct mmu_ctx *ctx)
{
	list_move_tail(&ctx->node, &ctx_used);
}

static inline void mmu_init_ctx_lists(void)
{
	INIT_LIST_HEAD(&ctx_free);
	INIT_LIST_HEAD(&ctx_used);
}

static inline struct mmu_ctx *mmu_find_ctx(unsigned int ctx_num)
{
	struct mmu_ctx *p_elem;


	if (ctx_num > _mmu_num_ctp)
		return NULL;

	list_for_each_entry(p_elem, &ctx_used, node) {
		if (p_elem->ctx_num == ctx_num)
			return p_elem;
	}

	list_for_each_entry(p_elem, &ctx_free, node) {
		if (p_elem->ctx_num == ctx_num)
			return p_elem;
	}


	return NULL;
}


/**
 * @brief set the current working context
 */

static inline int mmu_set_current_ctx(unsigned int ctx_num)
{
	struct mmu_ctx *ctx;

	ctx = mmu_find_ctx(ctx_num);
	if (!ctx)
		return -EINVAL;

	_mmu_ctx = ctx;

	return 0;
}


/**
 * @brief get the current working context
 */

static inline struct mmu_ctx *mmu_get_current_ctx(void)
{
	return _mmu_ctx;
}


/**
 * @brief add a new working context
 */

static inline void mmu_add_ctx(unsigned long ptd)
{
	_mmu_ctp[_mmu_num_ctp] = ptd;
	_mmu_num_ctp++;
}

/**
 * @brief get the current number of registered contexts
 *
 * @returns the current number of registered contexts
 */

static inline unsigned int mmu_get_num_ctx(void)
{
	return _mmu_num_ctp;
}

/**
 * @brief set the context table pointer
 * @param addr a pointer to the table
 */

static inline void mmu_set_ctp(unsigned long *addr)
{
	_mmu_ctp = addr;
	srmmu_set_ctx_tbl_addr((unsigned long) _mmu_ctp);
}


/**
 * @brief 1:1 map the full 32 bit space
 *
 * @param ctx_num the context number to configure
 */

static inline void mmu_set_map_full(unsigned int ctx_num)
{
	if (ctx_num >= SRMMU_CONTEXTS)
		return;

	_mmu_ctp[ctx_num] = SRMMU_PTE(0x0,
				      (SRMMU_CACHEABLE | SRMMU_ACC_S_RWX_2));
}


/**
 * @brief initialise all lvl1 table entries as invalid
 */

static void mmu_set_lvl1_tbl_invalid(struct srmmu_ptde *ptde)
{
	int i;

	for (i = 0; i < SRMMU_SIZE_TBL_LVL_1; i++)
	       ptde[i].pte = SRMMU_ENTRY_TYPE_INVALID;
}


/**
 * @brief initialise all lvl2 table entries as invalid
 */

static void mmu_set_lvl2_tbl_invalid(struct srmmu_ptde *ptde)
{
	int i;

	for (i = 0; i < SRMMU_SIZE_TBL_LVL_2; i++)
	       ptde[i].pte = SRMMU_ENTRY_TYPE_INVALID;
}


/**
 * @brief initialise all lvl3 table entries as invalid
 */

static void mmu_set_lvl3_tbl_invalid(struct srmmu_ptde *ptde)
{
	int i;

	for (i = 0; i < SRMMU_SIZE_TBL_LVL_3; i++)
	       ptde[i].pte = SRMMU_ENTRY_TYPE_INVALID;
}


/**
 * @brief increment page reference counter
 *
 * @returns current reference count
 */

static unsigned long mmu_inc_ref_cnt(struct srmmu_ptde *tbl,
					unsigned long tbl_size)
{
	unsigned long cnt;

	unsigned long *ptd;


	if (IS_MAGIC(tbl[-1].ptd))
		ptd = &tbl[-2].ptd;
	else if (IS_MAGIC(tbl[tbl_size].ptd))
		ptd = &tbl[tbl_size + 1].ptd;
	else
		BUG();


	cnt = REF_FROM_MAGIC((*ptd)) + 1;
	(*ptd) = MAGIC_REF_CNT(cnt);

	return cnt;
}


/**
 * @brief decrement page reference counter
 *
 * @returns current reference count
 */

static unsigned long mmu_dec_ref_cnt(struct srmmu_ptde *tbl,
					unsigned long tbl_size)
{
	unsigned long cnt;

	unsigned long *ptd;


	if (IS_MAGIC(tbl[-1].ptd))
		ptd = &tbl[-2].ptd;
	else if (IS_MAGIC(tbl[tbl_size].ptd))
		ptd = &tbl[tbl_size + 1].ptd;
	else
		BUG();


	cnt = REF_FROM_MAGIC((*ptd));
	if (!cnt)
		BUG();

	cnt = cnt - 1;

	(*ptd) = MAGIC_REF_CNT(cnt);

	return cnt;
}


/**
 * @brief allocate and align a SRMMU page table
 */

static struct srmmu_ptde *mmu_alloc_tbl(struct mmu_ctx *ctx,
					unsigned long tbl_size)
{
	int offset;

	struct srmmu_ptde *ptde;
	struct srmmu_ptde *ptde_align;


	ptde = (struct srmmu_ptde *) ctx->alloc(2 * tbl_size);
	if (!ptde)
		return NULL;

	ptde_align = ALIGN_PTR(ptde, tbl_size);

	/* store positive offset as a ptd */
	offset = (int) ptde_align - (int) ptde;
	if (offset > MAGIC_WORDS) {
		ptde_align[-1].ptd = MAGIC(offset);
		ptde_align[-2].ptd = MAGIC_REF_CNT(0);
	} else {
		ptde_align[tbl_size].ptd = MAGIC(0);
		ptde_align[tbl_size + 1].ptd = MAGIC_REF_CNT(0);
	}

	return ptde_align;
}


/**
 * @brief free a table
 */

static void mmu_free_tbl(struct mmu_ctx *ctx, struct srmmu_ptde *tbl,
			 unsigned long tbl_size)
{
	unsigned long ptd;

	void *addr;


	if (IS_MAGIC(tbl[-1].ptd))
		ptd = tbl[-1].ptd;
	else if (IS_MAGIC(tbl[tbl_size].ptd))
		ptd = tbl[tbl_size].ptd;
	else
		BUG();

	addr = (void *) ((int) tbl - OFFSET_FROM_MAGIC(ptd));

	ctx->free(addr);
}

/**
 * @brief allocate a level 1 table
 */

static struct srmmu_ptde *mmu_alloc_lvl1_tbl(struct mmu_ctx *ctx)
{
	struct srmmu_ptde *ptde;


	ptde = mmu_alloc_tbl(ctx, SRMMU_TBL_LVL_1_ALIGN);
	if (!ptde)
		return NULL;

	mmu_set_lvl1_tbl_invalid(ptde);

	return ptde;
}


/**
 * @brief look up a level 2 table by virtual address
 */

static struct srmmu_ptde *mmu_find_tbl_lvl2(struct mmu_ctx *ctx,
					    unsigned long va)
{
	unsigned long page;

	struct srmmu_ptde *lvl1;


	lvl1 = ctx->tbl.lvl1;

	page = SRMMU_LVL1_GET_TBL_OFFSET(va);

	if (lvl1[page].pte & SRMMU_ENTRY_TYPE_PT_ENTRY)
		return NULL;

	if (lvl1[page].pte == SRMMU_ENTRY_TYPE_INVALID)
		return NULL;

	return (struct srmmu_ptde *) SRMMU_PTD_TO_ADDR(lvl1[page].ptd);
}


/**
 * @brief look up a level 3 table by virtual address
 */

static struct srmmu_ptde *mmu_find_tbl_lvl3(struct mmu_ctx *ctx,
					    unsigned long va)
{
	unsigned long page;

	struct srmmu_ptde *lvl2;


	lvl2 = mmu_find_tbl_lvl2(ctx, va);
	if (!lvl2)
		goto no_table;

	page = SRMMU_LVL2_GET_TBL_OFFSET(va);

	if (lvl2[page].pte & SRMMU_ENTRY_TYPE_PT_ENTRY)
		goto no_table;

	if (lvl2[page].pte == SRMMU_ENTRY_TYPE_INVALID)
		goto no_table;


	return (struct srmmu_ptde *) SRMMU_PTD_TO_ADDR(lvl2[page].ptd);

no_table:
	return NULL;
}


/**
 * @brief if necessary, allocate a new level 2 table
 *
 * @returns 0 on success, otherwise error
 */

static int mmu_need_lvl2_tbl(struct mmu_ctx *ctx, unsigned long va)
{
	unsigned long pg_lvl1;

	struct srmmu_ptde *ptde;


	pg_lvl1 = SRMMU_LVL1_GET_TBL_OFFSET(va);

	/* lvl 1 page in use, no can do */
	if (ctx->tbl.lvl1[pg_lvl1].pte & SRMMU_ENTRY_TYPE_PT_ENTRY)
		return -EINVAL;

	if (ctx->tbl.lvl1[pg_lvl1].pte != SRMMU_ENTRY_TYPE_INVALID)
		return 0;


	ptde = mmu_alloc_tbl(ctx, SRMMU_TBL_LVL_2_ALIGN);
	if (!ptde)
		return -ENOMEM;

	mmu_set_lvl2_tbl_invalid(ptde);

	/* point entry to the new lvl2 table */
	ctx->tbl.lvl1[pg_lvl1].ptd = SRMMU_PTD((unsigned long) ptde);

	/* we don't count lvl 2 references in lvl1 tables */

	pr_debug("SRMMU: allocated new lvl2 table\n");

	return 0;
}


/**
 * @brief if necessary, allocate a new level 2 table
 *
 * @returns 0 on success, otherwise error
 */

static int mmu_need_lvl3_tbl(struct mmu_ctx *ctx, unsigned long va)
{
	int ret;

	unsigned long pg_lvl2;

	struct srmmu_ptde *lvl2;
	struct srmmu_ptde *lvl3;


	/* see if we need a lvl3 table */
	lvl3 = mmu_find_tbl_lvl3(ctx, va);
	if (lvl3)
		return 0;

	/* we might need a lvl2 table first */
	lvl2 = mmu_find_tbl_lvl2(ctx, va);
	if (!lvl2) {
		if((ret = mmu_need_lvl2_tbl(ctx, va)))
			return ret;

		lvl2 = mmu_find_tbl_lvl2(ctx, va);
	}

	/* allocate a lvl3 table */
	lvl3 = mmu_alloc_tbl(ctx, SRMMU_TBL_LVL_3_ALIGN);
	if (!lvl3)
		return -ENOMEM;

	mmu_set_lvl3_tbl_invalid(lvl3);

	/* point lvl2 entry to the new lvl3 table */
	pg_lvl2 = SRMMU_LVL2_GET_TBL_OFFSET(va);
	lvl2[pg_lvl2].ptd = SRMMU_PTD((unsigned long) lvl3);

	mmu_inc_ref_cnt(lvl2, SRMMU_TBL_LVL_2_ALIGN);

	pr_debug("SRMMU: allocated new lvl3 table\n");

	return 0;
}


/**
 * @brief create a new SRMMU context
 *
 * @param alloc a pointer to a function we can use to allocate MMU context
 *	  tables
 * @param free a pointer to a function that returns MMU context tables to
 *	  the allocator
 *
 *
 * @return >= 0: number of created context, otherwise error
 *
 * @note allows up to SRMMU_CONTEXTS
 */

int srmmu_new_ctx(void *(*alloc)(size_t size), void  (*free)(void *addr))
{
	struct mmu_ctx *ctx;


	if (mmu_get_num_ctx() > SRMMU_CONTEXTS)
		return -ENOMEM;

	ctx = (struct mmu_ctx *) alloc(sizeof(struct mmu_ctx));
	if (!ctx)
		return -ENOMEM;


	ctx->alloc = alloc;
	ctx->free  = free;

	ctx->tbl.lvl1 = mmu_alloc_lvl1_tbl(ctx);
	if (!ctx->tbl.lvl1) {
		free(ctx);
		return -ENOMEM;
	}

	ctx->ctx_num = mmu_get_num_ctx();

	mmu_add_ctx(SRMMU_PTD((unsigned long) &ctx->tbl.lvl1[0]));

	mmu_ctx_add_free(ctx);

	return ctx->ctx_num;
}

/**
 * @brief release lvl3 pages
 *
 * @returns 1 if lvl 3 directory still exists, 0 if it was released
 */

static int srmmu_release_lvl3_pages(struct mmu_ctx *ctx,
				    unsigned long va, unsigned long va_end,
				    void (*free_page)(void *addr))
{
	unsigned long page;

	struct srmmu_ptde *lvl3;


	lvl3 = mmu_find_tbl_lvl3(ctx, va);
	if (!lvl3)
		return 0;

	/* limit to single lvl3 table */
	if (va_end > ALIGN(va + 1, SRMMU_MEDIUM_PAGE_SIZE))
		va_end = ALIGN(va + 1, SRMMU_MEDIUM_PAGE_SIZE);

	for ( ; va < va_end; va += SRMMU_SMALL_PAGE_SIZE) {


		page = SRMMU_LVL3_GET_TBL_OFFSET(va);

		/* it is quite possible that this is not mapped */
		if (lvl3[page].pte == SRMMU_ENTRY_TYPE_INVALID) {
			pr_debug("SRMMU: tried to release address 0x%08x (%lx), but "
				 "lvl3 page was marked invalid, ignoring.\n",
				 va, SRMMU_PTE_TO_ADDR(lvl3[page].pte));
			continue;
		}

		if (lvl3[page].pte & SRMMU_ENTRY_TYPE_PT_ENTRY) {

			free_page((void *) SRMMU_PTE_TO_ADDR(lvl3[page].pte));

			pr_debug("SRMMU: freed physical page %lx (va %lx) [%d][%d][%d]\n",
				 SRMMU_PTE_TO_ADDR(lvl3[page].pte),
				 va, SRMMU_LVL1_GET_TBL_OFFSET(va),
				 SRMMU_LVL2_GET_TBL_OFFSET(va),
				 SRMMU_LVL3_GET_TBL_OFFSET(va));

			lvl3[page].pte = SRMMU_ENTRY_TYPE_INVALID;

			if (!mmu_dec_ref_cnt(lvl3, SRMMU_TBL_LVL_3_ALIGN)) {
				mmu_free_tbl(ctx, lvl3, SRMMU_TBL_LVL_3_ALIGN);
				pr_debug("SRMMU: released lvl3 table\n");

				return 0;
			}
		}


	}

	return 1;
}


/**
 * @brief recursively release lvl2 pages
 *
 * @returns 1 if lvl 2 directory still exists, 0 if it was released
 */

static int srmmu_release_lvl2_pages(struct mmu_ctx *ctx,
				     unsigned long va, unsigned long va_end,
				     void (*free_page)(void *addr))
{
	unsigned long page;

	struct srmmu_ptde *lvl2;


	lvl2 = mmu_find_tbl_lvl2(ctx, va);

	if (!lvl2)
		return 0;

	/* limit to single lvl2 table */
	if (va_end > ALIGN(va + 1, SRMMU_LARGE_PAGE_SIZE))
		va_end = ALIGN(va + 1, SRMMU_LARGE_PAGE_SIZE);

	for ( ; va < va_end; va += SRMMU_MEDIUM_PAGE_SIZE) {

		page = SRMMU_LVL2_GET_TBL_OFFSET(va);

		/* medium mapping not used, its all done via small pages */

		/* it is quite possible that this is not mapped */
		if (lvl2[page].pte == SRMMU_ENTRY_TYPE_INVALID) {
			pr_debug("SRMMU: tried to release address 0x%08x, but "
				 "lvl2 page was marked invalid, ignoring.\n",
				 va);
			continue;
		}

		if (!srmmu_release_lvl3_pages(ctx, va, va_end, free_page)) {
			lvl2[page].pte = SRMMU_ENTRY_TYPE_INVALID;

			pr_debug("SRMMU: lvl3 table unreferenced\n");


			/* no more subtables, free ourselves */
			if (!mmu_dec_ref_cnt(lvl2, SRMMU_TBL_LVL_2_ALIGN)) {
				mmu_free_tbl(ctx, lvl2, SRMMU_TBL_LVL_2_ALIGN);
				pr_debug("SRMMU: released lvl2 table\n");

				return 0;
			}
		}

		/* make sure this is aligned to the next boundary, this
		 * is needed for the first section in particular
		 */
		va = ALIGN(va - SRMMU_MEDIUM_PAGE_SIZE + 1, SRMMU_MEDIUM_PAGE_SIZE);

	}

	return 1;
}


/**
 * @brief recursively release lvl1 pages
 */

static void srmmu_release_lvl1_pages(struct mmu_ctx *ctx,
				     unsigned long va, unsigned long va_end,
				     void (*free_page)(void *addr))
{

	unsigned long page;

	struct srmmu_ptde *lvl1;


	lvl1 = ctx->tbl.lvl1;

	for ( ; va < va_end; va += SRMMU_LARGE_PAGE_SIZE) {

		page = SRMMU_LVL1_GET_TBL_OFFSET(va);

		/* large mapping */
		if (lvl1[page].pte & SRMMU_ENTRY_TYPE_PT_ENTRY) {
			free_page((void *) SRMMU_PTE_TO_ADDR(lvl1[page].pte));
			lvl1[page].pte = SRMMU_ENTRY_TYPE_INVALID;
			continue;
		}

		if (lvl1[page].pte == SRMMU_ENTRY_TYPE_INVALID) {
			pr_debug("SRMMU: tried to release address 0x%08x, but "
				  "lvl1 page was marked invalid, skipping.\n",
				  va);
			continue;
		}

		if (!srmmu_release_lvl2_pages(ctx, va, va_end, free_page)) {
			lvl1[page].pte = SRMMU_ENTRY_TYPE_INVALID;
			pr_debug("SRMMU: lvl2 table unreferenced\n");
		}

		/* make sure this is aligned to the next boundary, this
		 * is needed for the first section in particular
		 */
		va = ALIGN(va - SRMMU_LARGE_PAGE_SIZE + 1, SRMMU_LARGE_PAGE_SIZE);
	}

	leon_flush_tlb_all();
}


/**
 * @brief recursively release pages by address
 *
 * @param ctx_num the context number
 * @param va the start virtual address
 * @param va_end the end virtual address
 * @parma free_page a function pages are returned to
 *
 * @note the addresses are assumed aligned to the page size
 */

void srmmu_release_pages(unsigned long ctx_num,
			 unsigned long va, unsigned long va_end,
			 void  (*free_page)(void *addr))
{
	struct mmu_ctx *ctx;

	if (va_end <= va)
		return;

	ctx = mmu_find_ctx(ctx_num);

	srmmu_release_lvl1_pages(ctx, va, va_end, free_page);
}


/**
 * @brief map a 16 MiB page from a virtual address to a physical address
 *
 * @param ctx_num the context number to do the mapping in
 * @param va the virtual address
 * @param pa the physical address
 * @parma perm the permissions to configure for the page
 *
 * @note the addresses are assumed to be aligned to the requested page size
 *
 * @return 0 on success
 */

int srmmu_do_large_mapping(unsigned long ctx_num,
			   unsigned long va, unsigned long pa,
			   unsigned long perm)
{
	size_t page;

	struct mmu_ctx *ctx;


	if (mmu_get_num_ctx() < ctx_num)
		return -EINVAL;


	ctx = mmu_find_ctx(ctx_num);

	if (!ctx)
		return -ENOMEM;

	page = SRMMU_LVL1_GET_TBL_OFFSET(va);
	ctx->tbl.lvl1[page].pte = SRMMU_PTE(pa, perm);

	leon_flush_cache_all();
	leon_flush_tlb_all();

	pr_debug("SRMMU: mapped 16 MiB page from 0x%08x to 0x%08x "
		 "of context %d, permissions 0x%08x\n",
		 va, pa, ctx->ctx_num, perm);

	return 0;
}


/**
 * @brief map a virtual address range to a physical address range in 16MiB pages
 *
 * @param ctx_num the context number to do the mapping in
 * @param va the virtual address
 * @param pa the physical address
 * @param pages the number of SRMMU_LARGE_PAGE_SIZE pages
 * @parma perm the permissions to configure for the pages
 *
 * @return 0 on success
 */

int srmmu_do_large_mapping_range(unsigned long ctx_num,
				 unsigned long va, unsigned long pa,
				 unsigned long num_pages, unsigned long perm)
{
	int ret;

	unsigned long i;


	if (mmu_get_num_ctx() < ctx_num)
		return -EINVAL;


	for (i = 0; i < num_pages; i++) {
		ret = srmmu_do_large_mapping(ctx_num,
					     va + i * SRMMU_LARGE_PAGE_SIZE,
					     pa + i * SRMMU_LARGE_PAGE_SIZE,
					     perm);
		if (ret) {
			pr_crit("SRMMU: failed to map %d pages from "
				"[0x%08lx, 0x%08lx] to [0x%08lx, 0x%08lx]\n",
				(num_pages - i),
				va + i * SRMMU_LARGE_PAGE_SIZE,
				va + num_pages * SRMMU_LARGE_PAGE_SIZE,
				pa + i * SRMMU_LARGE_PAGE_SIZE,
				pa + num_pages * SRMMU_LARGE_PAGE_SIZE);
			return ret;
		}
	}

	return 0;
}



/**
 * @brief map a 4 kiB page from a virtual address to a physical address
 *
 * @param ctx_num the context number to do the mapping in
 * @param va the virtual address
 * @param pa the physical address
 * @parma perm the permissions to configure for the page
 *
 * @note the addresses are assumed to be aligned to the requested page size
 *
 * @return 0 on success
 */

int srmmu_do_small_mapping(unsigned long ctx_num,
			   unsigned long va, unsigned long pa,
			   unsigned long perm)
{
	int ret;

	unsigned long pg;
	struct mmu_ctx *ctx;
	struct srmmu_ptde *lvl3;



	if (mmu_get_num_ctx() < ctx_num)
		return -EINVAL;

	ctx = mmu_find_ctx(ctx_num);

	if (!ctx)
		return -ENOMEM;


	ret = mmu_need_lvl3_tbl(ctx, va);
	if (ret)
		return ret;

	lvl3 = mmu_find_tbl_lvl3(ctx, va);

	pg = SRMMU_LVL3_GET_TBL_OFFSET(va);

	lvl3[pg].pte = SRMMU_PTE(pa, perm);

	mmu_inc_ref_cnt(lvl3, SRMMU_TBL_LVL_3_ALIGN);

	leon_flush_cache_all();
	leon_flush_tlb_all();

	pr_debug("SRMMU: mapped 4 kiB page from 0x%08x to 0x%08x "
		 "of context %d, permissions 0x%08x\n",
		 va, pa, ctx->ctx_num, perm);

	return 0;
}

/**
 * @brief map a virtual address range to a physical address range in 4kiB pages
 *
 * @param ctx_num the context number to do the mapping in
 * @param va the virtual address
 * @param pa the physical address
 * @param pages the number of SRMMU_SMALL_PAGE_SIZE pages
 * @parma perm the permissions to configure for the pages
 *
 * @return 0 on success
 */

int srmmu_do_small_mapping_range(unsigned long ctx_num,
				 unsigned long va, unsigned long pa,
				 unsigned long num_pages, unsigned long perm)
{
	int ret;

	unsigned long i;


	if (mmu_get_num_ctx() < ctx_num)
		return -EINVAL;


	for (i = 0; i < num_pages; i++) {
		ret = srmmu_do_small_mapping(ctx_num,
					     va + i * SRMMU_SMALL_PAGE_SIZE,
					     pa + i * SRMMU_SMALL_PAGE_SIZE,
					     perm);
		if (ret) {
			pr_crit("SRMMU: failed to map %d pages from "
				"[0x%08lx, 0x%08lx] to [0x%08lx, 0x%08lx]\n",
				(num_pages - i),
				va + i * SRMMU_SMALL_PAGE_SIZE,
				va + num_pages * PAGE_SIZE,
				pa + i * SRMMU_SMALL_PAGE_SIZE,
				pa + num_pages * PAGE_SIZE);
			return ret;
		}
	}

	return 0;
}


/**
 * @brief get the physical address of the page mapped to a virtual address
 */

unsigned long srmmu_get_pa_page(unsigned long ctx, unsigned long va)
{
	unsigned long page;

	struct srmmu_ptde *lvl3;


	lvl3 = mmu_find_tbl_lvl3(mmu_find_ctx(ctx), va);
	if (!lvl3)
		return 0;

	page = SRMMU_LVL3_GET_TBL_OFFSET(va);

	return SRMMU_PTE_TO_ADDR(lvl3[page].pte);
}




/**
 * @brief select a MMU context
 *
 * @param ctx_num the context number to select
 *
 * @return 0 on success, otherwise error
 */

int srmmu_select_ctx(unsigned long ctx_num)
{
	if (mmu_get_num_ctx() < ctx_num)
		return -EINVAL;

	if(mmu_set_current_ctx(ctx_num))
		return -EINVAL;

	srmmu_set_ctx(ctx_num);

	leon_flush_cache_all();
	leon_flush_tlb_all();

	return 0;
}


/**
 * @brief enable MMU operation
 */

void srmmu_enable_mmu(void)
{
	srmmu_set_mmureg(0x00000001);
	leon_flush_cache_all();
}


/**
 * @brief basic initialisation of the MMU
 *
 * @param alloc a pointer to a function we can use to allocate MMU context
 *	  tables
 * @param free a pointer to a function that returns MMU context tables to
 *	  the allocator
 *
 * @return 0 on success, otherwise error
 *
 * @note requires at least one mapping and a call to srmmu_enable_mmu()
 *	 to function
 */

	unsigned long *ctp;
void srmmu_init_per_cpu(void)
{
	mmu_set_ctp(ctp);
	mmu_set_current_ctx(0);
	srmmu_select_ctx(0);
	srmmu_enable_mmu();
}


int srmmu_init(void *(*alloc)(size_t size), void  (*free)(void *addr))
{
	int ret;

	unsigned int i;


	struct mmu_ctx *ctx;

	const size_t ctp_size = SRMMU_CONTEXTS * sizeof(unsigned long);


	/* don't call this twice by accident, we don't support multiple
	 * context table pointer tables
	 */
	BMP();

	mmu_init_ctx_lists();

	/* allocate twice the size of the table, so we can align it to the
	 * a boundary equal its own size
	 */
	ctp = (unsigned long *) alloc(2 * ctp_size);
	if (!ctp)
		return -ENOMEM;

	ctp = ALIGN_PTR(ctp, ctp_size);

	mmu_set_ctp(ctp);

	/* all contexts are invalid by default */
	for (i = 0; i < SRMMU_CONTEXTS; i++)
		ctp[i] = SRMMU_ENTRY_TYPE_INVALID;


	ret = srmmu_new_ctx(alloc, free);
	if(ret)
		return ret;

	BUG_ON(mmu_set_current_ctx(0));

	ctx = mmu_get_current_ctx();

	srmmu_select_ctx(0);

	return 0;
}
