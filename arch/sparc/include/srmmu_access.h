/**
 * @brief SPARC Reference (SR) Memory Management Unit (MMU) access functions
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @see SPARCv8 Architecture Manual for more info
 */

#ifndef _SPARC_SRMMU_ACCESS_H_
#define _SPARC_SRMMU_ACCESS_H_

unsigned int srmmu_get_mmu_ctrl(void);
struct srmmu_fault_status srmmu_get_mmu_fault_status(void);
unsigned int srmmu_get_mmu_fault_address(void);
unsigned int srmmu_get_mmu_impl(void);
unsigned int srmmu_get_mmu_ver(void);


void srmmu_set_ctx_tbl_addr(unsigned long addr);
void srmmu_set_ctx(unsigned int ctx);

void leon_flush_cache_all(void);
void leon_flush_tlb_all(void);
void srmmu_set_mmureg(unsigned long regval);
unsigned int srmmu_get_mmureg(void);


#endif /*_SPARC_SRMMU_ACCESS_H_ */
