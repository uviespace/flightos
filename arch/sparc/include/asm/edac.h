/**
 * @file arch/sparc/include/asm/edac.h
 *
 * @brief LEON EDAC subsystem initialisation
 *
 * @ingroup edacsys
 *
 * Architecture-side entry point exposed to board / architecture setup code.
 * leon_edac_init() is implemented in arch/sparc/kernel/edac.c and registers
 * the LEON struct edac_dev backend with the generic kernel-layer EDAC
 * interface (kernel/edac.c). See the @ref edacsys group page.
 */

#ifndef _SPARC_ASM_EDAC_H_
#define _SPARC_ASM_EDAC_H_

/**
 * @brief initialise the EDAC subsystem on the LEON
 */
void leon_edac_init(void);

#endif /* _SPARC_ASM_EDAC_H_ */
