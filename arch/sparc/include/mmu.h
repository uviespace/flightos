/**
 * @file arch/sparc/include/mmu.h
 * @ingroup kmem
 *
 * @brief SPARC MMU initialisation, context, trap handling and translation
 */

#ifndef _SPARC_MMU_H_
#define _SPARC_MMU_H_


/**
 * @brief initialise paging on the SRMMU
 */
void mm_mmu_paging_init(void);

/**
 * @brief handle an MMU trap (data access exception)
 *
 * Reads the MMU fault status and address registers, logs the fault
 * and, for invalid-address faults below the system break, allocates and
 * maps a page on demand.
 */
void mm_mmu_trap(void);

/**
 * @brief set the active MMU context
 *
 * @param ctx the MMU context to set
 *
 * @return 0 on success, otherwise error
 */
int mm_set_mmu_ctx(unsigned long ctx);

/**
 * @brief get the active MMU context
 *
 * @return the active MMU context number
 */
unsigned long mm_get_mmu_ctx(void);

/**
 * @brief translate a virtual address to its physical address
 *
 * @param va the virtual address
 *
 * @return the corresponding physical address, including the page offset
 *	 of va
 */
unsigned long mm_get_physical_addr(unsigned long va);


#endif /* _SPARC_MMU_H_ */
