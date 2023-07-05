/**
 * @file  arch/sparc/kernel/ahb.c
 * @ingroup ahb
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @defgroup ahb Advanced High-performance Bus (AHB)
 * @brief Access to the AHB registers
 *
 *
 * ## Overview
 *
 * This components implements functionality to access or modify the AHB status
 * registers of the GR712RC.
 * @see _GR712-UM v2.7 chapter 7_
 *
 * ## Mode of Operation
 *
 * None
 *
 * ## Error Handling
 *
 * None
 *
 * ## Notes
 *
 * None
 *
 */


#include <ahb.h>
#include <asm/io.h>
#include <asm/leon.h>
#include <asm/leon_reg.h>


/**
 * @brief deassert the new error bit in the AHB status register
 */

void ahbstat_clear_new_error(void)
{
#ifdef CONFIG_LEON3
	uint32_t tmp;

	struct leon3_ahbstat_registermap *ahbstat =
		(struct leon3_ahbstat_registermap *) LEON3_BASE_ADDRESS_AHBSTAT;


	tmp  = ioread32be(&ahbstat->status);
	tmp &= ~LEON3_AHB_STATUS_NE;
	iowrite32be(tmp, &ahbstat->status);
#endif /* CONFIG_LEON3 */
}


/**
 * @brief retrieve the AHB status register
 *
 * @return the contents of the AHB status register
 */

uint32_t ahbstat_get_status(void)
{
#ifdef CONFIG_LEON3
	struct leon3_ahbstat_registermap const *ahbstat =
		(struct leon3_ahbstat_registermap *) LEON3_BASE_ADDRESS_AHBSTAT;


	return ioread32be(&ahbstat->status);
#else /* CONFIG_LEON3 */
	return 0;
#endif
}


/**
 * @brief check the new error bit in the AHB status register
 *
 * @return not 0 if new error bit is set
 */

uint32_t ahbstat_new_error(void)
{
#ifdef CONFIG_LEON3
	struct leon3_ahbstat_registermap const *ahbstat =
		(struct leon3_ahbstat_registermap *) LEON3_BASE_ADDRESS_AHBSTAT;


	return (ioread32be(&ahbstat->status) & LEON3_AHB_STATUS_NE);
#else /* CONFIG_LEON3 */
	return 0;
#endif

}


/**
 * @brief check if the last error reported via the AHB status register is
 *        correctable
 *
 * @return not 0 if correctable error bit is set
 *
 * @see GR712-UM v2.3 p. 71
 *
 */

uint32_t ahbstat_correctable_error(void)
{
#ifdef CONFIG_LEON3
	struct leon3_ahbstat_registermap const *ahbstat =
		(struct leon3_ahbstat_registermap *) LEON3_BASE_ADDRESS_AHBSTAT;


	return (ioread32be(&ahbstat->status) & LEON3_AHB_STATUS_CE);
#else /* CONFIG_LEON3 */
	return 0;
#endif
}


/**
 * @brief get the AHB failing address
 *
 * @return the HADDR signal of the AHB transaction that caused the error
 *
 * @see GR712-UM v2.3 p. 72
 *
 */

uint32_t ahbstat_get_failing_addr(void)
{
#ifdef CONFIG_LEON3
	struct leon3_ahbstat_registermap const *ahbstat =
		(struct leon3_ahbstat_registermap *) LEON3_BASE_ADDRESS_AHBSTAT;


	return ioread32be(&ahbstat->failing_address);
#else /* CONFIG_LEON3 */
	return 0;
#endif
}

