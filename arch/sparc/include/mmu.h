/**
 * @file arch/sparc/include/mmu.h
 */

#ifndef _SPARC_MMU_H_
#define _SPARC_MMU_H_


void mm_mmu_paging_init(void);

void mm_mmu_trap(void);

int mm_set_mmu_ctx(unsigned long ctx);
unsigned long mm_get_mmu_ctx(void);


#endif /* _SPARC_MMU_H_ */
