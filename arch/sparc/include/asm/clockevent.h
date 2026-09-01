/**
 * @file arch/sparc/include/asm/clockevent.h
 * @ingroup timing
 *
 * @brief SPARC clockevent framework initialization interface.
 */

#ifndef _SPARC_CLOCKEVENT_H_
#define _SPARC_CLOCKEVENT_H_


#include <kernel/kernel.h>



/**
 * @brief initialise the SPARC clockevent framework
 *
 * @note on LEON3/4, all general purpose timers are registered as
 *	 clock event devices
 */
void sparc_clockevent_init(void);

#endif /* _SPARC_CLOCKEVENT_H_ */
