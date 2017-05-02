/**
 * @file arch/sparc/include/init.h
 *
 * @ingroup sparc
 */

#ifndef _SPARC_INIT_H_
#define _SPARC_INIT_H_

#if defined(CONFIG_KERNEL_STACK_PAGES)
#define KERNEL_STACK_PAGES CONFIG_KERNEL_STACK_PAGES
#else
#define KERNEL_STACK_PAGES 8
#endif



void paging_init(void);


#endif /*_SPARC_INIT_H_ */
