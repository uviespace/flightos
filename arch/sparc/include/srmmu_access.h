/**
 * @brief SPARC Reference (SR) Memory Management Unit (MMU) access functions
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @ingroup srmmu
 *
 * @see SPARCv8 Architecture Manual for more info
 */

#ifndef _SPARC_SRMMU_ACCESS_H_
#define _SPARC_SRMMU_ACCESS_H_

/**
 * @brief access to the SRMMU control register
 *
 * @return SRMMU control register contents
 */
unsigned int srmmu_get_mmu_ctrl(void);

/**
 * @brief access to the SRMMU fault status register
 *
 * @return SRMMU fault status register contents
 */
struct srmmu_fault_status srmmu_get_mmu_fault_status(void);

/**
 * @brief access to the SRMMU fault address register
 *
 * @return SRMMU fault address register contents
 */
unsigned int srmmu_get_mmu_fault_address(void);

/**
 * @brief get the SRMMU implementation
 *
 * @return the implementation identifier
 */
unsigned int srmmu_get_mmu_impl(void);

/**
 * @brief get the SRMMU version
 *
 * @return the implementation version
 */
unsigned int srmmu_get_mmu_ver(void);


/**
 * @brief set the context table address in the MMU
 *
 * @param addr the address of the context table
 */
void srmmu_set_ctx_tbl_addr(unsigned long addr);

/**
 * @brief select the MMU context
 *
 * @param ctx the context to select
 */
void srmmu_set_ctx(unsigned int ctx);

/**
 * @brief flush all leon caches
 */
void leon_flush_cache_all(void);

/**
 * @brief flush the entire translation lookaside buffer
 */
void leon_flush_tlb_all(void);

/**
 * @brief write to the SRMMU control register
 *
 * @param regval the value to write to the MMU control register
 */
void srmmu_set_mmureg(unsigned long regval);


#endif /*_SPARC_SRMMU_ACCESS_H_ */
