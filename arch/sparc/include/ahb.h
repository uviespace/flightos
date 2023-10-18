/**
 * @file arch/sparc/include/leon3_ahb.h
 
 */

#ifndef _SPARC_AHB_H_
#define _SPARC_AHB_H_

#include <kernel/types.h>

/**
 * @see GR712-UM v2.3 p. 71
 */
#define LEON3_AHB_STATUS_HSIZE		0x00000007
#define LEON3_AHB_STATUS_HMASTER	0x00000078
#define LEON3_AHB_STATUS_HWRITE		0x00000080
#define LEON3_AHB_STATUS_NE		0x00000100
#define LEON3_AHB_STATUS_CE		0x00000200


void ahbstat_clear_new_error(void);

uint32_t ahbstat_get_status(void);
uint32_t ahbstat_new_error(void);
uint32_t ahbstat_correctable_error(void);
uint32_t ahbstat_get_failing_addr(void);

#endif /* _SPARC_AHB_H_ */
