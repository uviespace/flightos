/**
 * @file arch/sparc/include/irq.h
 */

#ifndef _SPARC_IRQ_H_
#define _SPARC_IRQ_H_

#include <asm/leon_reg.h>


void leon_irq_init(void);


/* in the LEON3, interrupts 1-15 are primary, 16-31 are extended */
#ifdef CONFIG_LEON3
#define LEON_WANT_EIRQ(x) (x)
#define LEON_REAL_EIRQ(x) (x)
#endif /* CONFIG_LEON3 */

/* in the LEON2, interrupts 1-15 are primary, 0-31 are extended, we treat them
 * as IRLs 16...n */
#ifdef CONFIG_LEON2
#define LEON_WANT_EIRQ(x) (LEON2_IRL_SIZE + (x))
#define LEON_REAL_EIRQ(x) ((x) - LEON2_IRL_SIZE)
#endif /* CONFIG_LEON2 */


#endif /* _SPARC_IRQ_H_ */
