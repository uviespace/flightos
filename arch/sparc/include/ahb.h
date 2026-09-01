/**
 * @file arch/sparc/include/ahb.h
 * @brief LEON3 AHB status register definitions
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


/**
 * @brief deassert the new error bit in the AHB status register
 */

void ahbstat_clear_new_error(void);

/**
 * @brief retrieve the AHB status register
 *
 * @return the contents of the AHB status register
 */

uint32_t ahbstat_get_status(void);

/**
 * @brief check the new error bit in the AHB status register
 *
 * @return not 0 if new error bit is set
 */

uint32_t ahbstat_new_error(void);

/**
 * @brief check if the last error reported via the AHB status register is
 *        correctable
 *
 * @return not 0 if correctable error bit is set
 */

uint32_t ahbstat_correctable_error(void);

/**
 * @brief get the AHB failing address
 *
 * @return the HADDR signal of the AHB transaction that caused the error
 */

uint32_t ahbstat_get_failing_addr(void);

#endif /* _SPARC_AHB_H_ */
