/**
 * @brief SRMMU register access functions
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @ingroup srmmu
 *
 * @note only LEON ASI is supported
 */



#include <uapi/asi.h>
#include <srmmu.h>

#include <kernel/kernel.h>


#define MMU_CTRL_REG	0x00000000	/* control register */
#define MMU_CTXP_REG	0x00000100	/* context pointer */
#define MMU_CTX_REG	0x00000200	/* context */
#define MMU_FLTS_REG	0x00000300	/* fault status */
#define MMU_FLTA_REG	0x00000400	/* fault address */



/**
 * @brief access to the SRMMU control register
 *
 * @return SR MMU control register contents
 */

unsigned int srmmu_get_mmu_ctrl(void)
{
	unsigned int mmu_ctrl;

	__asm__ __volatile__(
			"lda	[%%g0] " __stringify(ASI_LEON_MMUREGS) ", %0\n\t"
			: "=r" (mmu_ctrl)
			:
			:"memory");

	return mmu_ctrl;
}


/**
 * @brief access to the SRMMU fault status register
 *
 * @return SRMMU fault status register contents
 */

struct srmmu_fault_status srmmu_get_mmu_fault_status(void)
{
	struct srmmu_fault_status mmu_fault_status;

	__asm__ __volatile__(
			"lda	[%1] " __stringify(ASI_LEON_MMUREGS) ", %0\n\t"
			:"=r" (mmu_fault_status)
			:"r" (MMU_FLTS_REG)
			:"memory");

	return mmu_fault_status;
}

/**
 * @brief access to the SRMMU fault address register
 *
 * @return SRMMU fault address register contents
 */

unsigned int srmmu_get_mmu_fault_address(void)
{
	unsigned int mmu_fault_addr;

	__asm__ __volatile__(
			"lda	[%1] " __stringify(ASI_LEON_MMUREGS) ", %0\n\t"
			:"=r" (mmu_fault_addr)
			:"r" (MMU_FLTA_REG)
			:"memory");


	return mmu_fault_addr;
}


/**
 * @brief get the SRMMU implementation
 *
 * @return the implementation identifier
 */

unsigned int srmmu_get_mmu_impl(void)
{
	return (srmmu_get_mmu_ctrl() & SRMMU_CTRL_IMPL_MASK) >>
		SRMMU_CTRL_IMPL_SHIFT;
}


/**
 * @brief get the SRMMU implementation
 *
 * @return the implementation version
 */

unsigned int srmmu_get_mmu_ver(void)
{
	return (srmmu_get_mmu_ctrl() & SRMMU_CTRL_VER_MASK) >>
		SRMMU_CTRL_VER_SHIFT;
}


/**
 * @brief set the context table address in the MMU
 *
 * @param addr the address of the context table
 *
 * TODO: clean up magic
 */

void srmmu_set_ctx_tbl_addr(unsigned long addr)
{
	addr = ((addr >> 4) & 0xfffffff0);

	__asm__ __volatile__("sta %0, [%1] %2\n\t"
			     "flush          \n\t" : :
			     "r" (addr), "r" (MMU_CTXP_REG),
			     "i" (ASI_LEON_MMUREGS) :
			     "memory");
}


/**
 * @brief select the MMU contest
 *
 * @param ctx the context to select
 *
 * TODO: clean up magic
 */

void srmmu_set_ctx(unsigned int ctx)
{
	__asm__ __volatile__("sta %0, [%1] %2\n\t" : :
			     "r" (ctx), "r" (MMU_CTX_REG),
			     "i" (ASI_LEON_MMUREGS) : "memory");
}



/**
 * TODO
 */

void srmmu_set_mmureg(unsigned long regval)
{
	__asm__ __volatile__(
			"sta	%0, [%%g0] " __stringify(ASI_LEON_MMUREGS) "\n\t"
			:
			: "r" (regval)
			:"memory");

}


/**
 * @brief flush all leon caches
 
 * TODO: clean up magic, should be part of leon asm
 */

void leon_flush_cache_all(void)
{
	__asm__ __volatile__(" flush ");	/*iflush */
	__asm__ __volatile__("sta %%g0, [%%g0] %0\n\t" : :
			     "i"(ASI_LEON_DFLUSH) : "memory");
}


/**
 * @brief flush the the transation lookaside buffer
 *
 * TODO: clean up magic, should be part of leon asm
 */
void leon_flush_tlb_all(void)
{
	leon_flush_cache_all();
	__asm__ __volatile__("sta %%g0, [%0] %1\n\t" : : "r"(0x400),
			     "i"(ASI_LEON_MMUFLUSH) : "memory");
}
