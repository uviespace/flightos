#include <stdint.h>


#include <errno.h>
#include <kernel/edac.h>
#include <kernel/export.h>


static struct edac_dev *edac;


/**
 * @brief set a reset handler callback in case a double bit error occurs
 *	  in a criticl section
 */

void edac_set_reset_callback(void (*handler)(void *), void *userdata)
{
	if (edac->set_reset_handler)
		edac->set_reset_handler(handler, userdata);
}
EXPORT_SYMBOL(edac_set_reset_callback);


/**
 * @brief add a critical memory segment definition to the EDAC subsystem
 * @note a double bit error in this segment will lead to the
 *	 exectution of a supplied reset function
 *
 *
 * @returns 0 on success, otherwise error
 */

int edac_critical_segment_add(void *begin, void *end)
{
	if (edac->crit_seg_add)
		return edac->crit_seg_add(begin, end);

	return -EINVAL;
}
EXPORT_SYMBOL(edac_critical_segment_add);


/**
 * @brief remove a critical memory segment definition from the EDAC subsystem
 *
 * @returns 0 on success, otherwise error
 */

int edac_critical_segment_rem(void *begin, void *end)
{
	if (edac->crit_seg_rem)
		return edac->crit_seg_rem(begin, end);

	return -EINVAL;
}
EXPORT_SYMBOL(edac_critical_segment_rem);


/**
 * @brief disable the EDAC system
 */

void edac_disable(void)
{
	if (edac->disable)
		return edac->disable();
}
EXPORT_SYMBOL(edac_disable);


/**
 * @brief enable the EDAC system
 */

void edac_enable(void)
{
	if (edac->enable)
		edac->enable();

}
EXPORT_SYMBOL(edac_enable);


/**
 * @brief write a faulty value/edac check bit combination to a memory location
 *
 * @param addr		address pointer
 * @param mem_value	a 32-bit value to write to memory
 * @param edac_value	a 32-bit value to use as input for the calculation of
 *                      the edac checkbits
 *
 * @note mem_value and edac_value should be off by one or two bits to inject a
 *       single or double fault respectively, otherwise the outcome may be
 *       either
 */

void edac_inject_fault(void *addr, uint32_t mem_value, uint32_t edac_value)
{
	if (edac->inject_fault)
		edac->inject_fault(addr, mem_value, edac_value);
}


/**
 * @brief initialise the EDAC system
 */

void edac_init(struct edac_dev *dev)
{
	edac = dev;

	edac_enable();
}
