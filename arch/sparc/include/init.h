/**
 * @file arch/sparc/include/init.h
 *
 * @ingroup sparc
 *
 * @brief SPARC paging initialisation
 */

#ifndef _SPARC_INIT_H_
#define _SPARC_INIT_H_

#if defined(CONFIG_KERNEL_STACK_PAGES)
#define KERNEL_STACK_PAGES CONFIG_KERNEL_STACK_PAGES
#else
#define KERNEL_STACK_PAGES 8
#endif



/**
 * @brief initialise paging
 *
 * Sets up the boot memory allocator and, if an MMU is configured,
 * initialises the MMU (paging) subsystem.
 */
void paging_init(void);


#endif /*_SPARC_INIT_H_ */
