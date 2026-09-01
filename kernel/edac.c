/**
 * @file kernel/edac.c
 * @ingroup kmem
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @defgroup edacsys EDAC subsystem
 *
 * @brief kernel-level EDAC (Error Detection And Correction) support
 *
 * This module implements the high-level, architecture-independent kernel-side
 * EDAC interface. It contains no hardware-specific logic of its own: the
 * enable/disable, critical segment management, error status/clear, fault
 * injection, bypass reads and reset callback entry points forward to the
 * operations of a single, platform-provided struct edac_dev registered through
 * edac_init().
 *
 * ## Concept
 *
 * The EDAC subsystem is split into two layers:
 *   - the high-level abstracted interface, implemented in this file and
 *     declared in include/kernel/edac.h;
 *   - a platform-specific backend under arch/ that implements the operations
 *     of struct edac_dev and wires it into the generic layer with
 *     edac_init(&backend_dev).
 *
 * Only a LEON (SPARCv8) backend exists so far: arch/sparc/kernel/edac.c
 * provides struct edac_dev leon_edac and registers it from leon_edac_init(),
 * which is called by setup_arch() during boot. The relationship between the
 * two layers, the error-detection flow and the LEON-specific behaviour is
 * documented by the architecture section and the diagrams on this group page.
 *
 * ## Structure
 *
 * The generic interface is built around a single static pointer:
 *
 *   - edac                : the registered struct edac_dev; set by edac_init().
 *   - edac_init(dev)      : store the platform's struct edac_dev, then enable
 *                           EDAC through the backend's enable operation.
 *   - edac_enable()       : invoke the backend enable operation, if set.
 *   - edac_disable()      : invoke the backend disable operation, if set.
 *   - edac_critical_segment_add()/rem() : add/remove a critical memory segment
 *                           through the backend operations. Return -EINVAL if
 *                           the operation is unset.
 *   - edac_error_detected() : report whether an error was detected through the
 *                           backend operation. Returns 0 if it is unset.
 *   - edac_get_error_addr() : return the address of the last error through the
 *                           backend operation. Returns 0 if it is unset.
 *   - edac_error_clear()  : clear the error status through the backend
 *                           operation, if set.
 *   - edac_inject_fault() : inject a synthetic fault through the backend
 *                           operation, if set.
 *   - edac_bypass_read()  : read memory bypassing EDAC checkbits through the
 *                           backend operation. Returns 0xdeadcafe if unset.
 *   - edac_set_reset_callback() : install a reset handler through the backend
 *                           operation, if set.
 *
 * The observed C consumers (kernel/syscalls.c, kernel/memscrub.c and the
 * RAMSES / SMILE projects) call the exported functions above. The
 * implementation does not enforce whether a caller accesses the edac pointer
 * or struct edac_dev directly; direct-access policy needs review.
 *
 * @startuml FlightOS EDAC Subsystem
 * title EDAC Subsystem Dependencies
 *
 * !pragma layout ortho
 *
 * skinparam component {
 *   BackgroundColor #E8F5E9
 *   BorderColor #333333
 * }
 * skinparam arrowColor #333333
 *
 * package "Kernel Core (kernel/)" {
 *   [memscrub.c] as MEMSCRUB
 *   [syscalls.c] as SYSCALLS
 *   [edac.c] as EDAC_API
 * }
 *
 * package "Abstraction" {
 *   [struct edac_dev] as EDAC_DEV
 * }
 *
 * package "Architecture (arch/sparc/)" {
 *   [edac.c] as EDAC_ARCH
 *   [leon3_memcfg.c] as MEMCFG
 *   [ahb.c] as AHB
 *   [traps.c] as TRAPS
 * }
 *
 * SYSCALLS -down-> EDAC_API : edac_inject_fault()
 * MEMSCRUB -down-> EDAC_API : edac functions
 *
 * EDAC_API -down-> EDAC_DEV : holds pointer\nfunction callbacks
 *
 * EDAC_DEV -down-> EDAC_ARCH : implements
 * EDAC_ARCH -down-> MEMCFG : memcfg operations
 * EDAC_ARCH -down-> AHB : AHB bus access
 * EDAC_ARCH -down-> TRAPS : trap registration
 *
 * note bottom of EDAC_DEV
 *   edac_dev struct provides:
 *   enable/disable
 *   crit_seg_add/rem
 *   error_detected/get/clear
 *   inject_fault
 *   bypass_read
 *   set_reset_handler
 * end note
 *
 * @enduml
 *
 * @startuml FlightOS EDAC Generic API Flow
 * title Generic EDAC API Flow
 *
 * skinparam activityBackgroundColor #FFF3E0
 * skinparam activityBorderColor #333333
 *
 * |edac_init()|
 * start
 * :edac = dev;
 * :edac_enable();
 * stop
 *
 * |edac_enable() / edac_disable()|
 * start
 * if (backend op set) then (yes)
 *   :invoke backend\nenable/disable op;
 * else (op unset)
 * endif
 * stop
 *
 * |edac_critical_segment_add() / rem()|
 * start
 * if (backend op set) then (yes)
 *   :invoke backend\ncrit_seg_add/rem;
 *   :return backend result;
 * else (op unset)
 *   :return -EINVAL;
 * endif
 * stop
 *
 * |edac_error_detected()|
 * start
 * if (backend op set) then (yes)
 *   :invoke backend\nerror_detected;
 * else (op unset)
 *   :return 0;
 * endif
 * stop
 *
 * |edac_get_error_addr()|
 * start
 * if (backend op set) then (yes)
 *   :invoke backend\nget_error_addr;
 * else (op unset)
 *   :return 0;
 * endif
 * stop
 *
 * |edac_error_clear()|
 * start
 * if (backend op set) then (yes)
 *   :invoke backend\nerror_clear;
 * else (op unset)
 * endif
 * stop
 *
 * |edac_inject_fault()|
 * start
 * if (backend op set) then (yes)
 *   :invoke backend\ninject_fault;
 * else (op unset)
 * endif
 * stop
 *
 * |edac_bypass_read()|
 * start
 * if (backend op set) then (yes)
 *   :invoke backend\nbypass_read;
 * else (op unset)
 *   :return 0xdeadcafe;
 * endif
 * stop
 *
 * |edac_set_reset_callback()|
 * start
 * if (backend op set) then (yes)
 *   :invoke backend\nset_reset_handler;
 * else (op unset)
 * endif
 * stop
 *
 * @enduml
 *
 */

#include <stdint.h>

#include <errno.h>
#include <kernel/edac.h>
#include <kernel/export.h>

static struct edac_dev *edac;


/**
 * @brief set a reset handler callback in case a double bit error occurs
 *        in a critical section
 *
 * @param handler	pointer to the reset handler function to be called
 * @param userdata	pointer passed as argument to @p handler when invoked
 */

void edac_set_reset_callback(void (*handler)(void *), void *userdata)
{
	if (edac->set_reset_handler)
		edac->set_reset_handler(handler, userdata);
}
EXPORT_SYMBOL(edac_set_reset_callback);


/**
 * @brief add a critical memory segment definition to the EDAC subsystem
 *
 * @param begin		pointer to the start of the critical memory segment
 * @param end		pointer to the end of the critical memory segment
 *
 * @note a double bit error in this segment will lead to the
 *       execution of a supplied reset function
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
 * @param begin		pointer to the start of the critical memory segment
 * @param end		pointer to the end of the critical memory segment
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
 * @brief check if an EDAC error flag has been raised
 *
 * @returns non-zero if an error has been detected, 0 otherwise or if
 *          functionality is unavailable
 */

int edac_error_detected(void)
{
	if (edac->error_detected)
		return edac->error_detected();

	return 0;
}
EXPORT_SYMBOL(edac_error_detected);


/**
 * @brief get the address of the last detected error
 *
 * @returns the memory address associated with the last detected error,
 *          0 if functionality is unavailable
 */

unsigned long edac_get_error_addr(void)
{
	if (edac->get_error_addr)
		return edac->get_error_addr();

	return 0;
}
EXPORT_SYMBOL(edac_get_error_addr);


/**
 * @brief clear the last EDAC error flag
 */

void edac_error_clear(void)
{
	if (edac->error_clear)
		edac->error_clear();
}
EXPORT_SYMBOL(edac_error_clear);


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
 * @brief read a memory location while bypassing error correction
 *
 * @param addr		address pointer
 *
 * @returns the value at the location, always 0xdeadcafe if functionality is
 *	    unavailable
 */

unsigned long edac_bypass_read(void *addr)
{
	if (edac->bypass_read)
		return edac->bypass_read(addr);

	return 0xdeadcafe;
}


/**
 * @brief initialise the EDAC system
 *
 * @param dev		pointer to an initialised edac_dev struct providing
 *			the backend operations for the EDAC subsystem
 */

void edac_init(struct edac_dev *dev)
{
	edac = dev;

	edac_enable();
}
