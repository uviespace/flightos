/* SPDX-License-Identifier: GPL-2.0 */

#include <asm/ttable.h>
#include <stack.h>


/* don't touch these */
#define t_psr        l0 /* set by trap entry */
#define t_pc         l1
#define t_npc        l2
#define t_wim        l3 /* set by trap entry */
#define t_tmp        l4
#define t_irqlvl     l7 /* set by trap entry */


/* at this time with nested IRQs, it takes trap + jump + save_all up to call:
 * 4 + 3 + 43(+33 worst case) + 7 = 57 (90 worst) cycles
 * the call is pc-relative, hence 2 cycles total
 * considering only the primary IRL and no statistics, leon_irq_dispatch will
 * take 24 cycles to enter the assigned ISR, i.e. 83 (116) cycles until the
 * IRQ can be serviced.
 *
 * only considering the time it takes to return from the ISR and assuming only
 * a single ISR is assigned to the IRL, leon_irq_dispatch will return
 * after 12 cycles and arrive at the call to schedule after a total of 13 cycles
 *
 * Assuming there is no scheduling event and not considering the internals of
 * the function, the overhead for call to and return from schedule() will take
 * a total of 4 cycles
 *
 * the psr is updated in 1 cycle, the call to restore_all will take
 * 39 (61 worst case) cycles
 *
 * The total time to service an IRQ is hence 150 cycles (195 worst case),
 * not considering an actual scheduling event or the run time of the ISR
 */


	.align	4
	.globl	__interrupt_entry

__interrupt_entry:
	SAVE_ALL

#ifdef CONFIG_SPARC_NESTED_IRQ
	/* The 4 bits of the PSR field set the level above which interrupts
	 * may occur. If we want ISR preemption, we set the level according
	 * to the IRQ number that occured...
	 */

	sll     %t_irqlvl, PSR_PIL_SHIFT, %g1	! shift into place
	andn    %t_psr, PSR_PIL, %g2		! clear field
	or      %g1, %g2, %g2			! set new level

#else /* !(CONFIG_SPARC_NESTED_IRQ)
	/* ... otherwise we just set it to max */

	or      %t_psr, PSR_PIL, %g2

#endif /* CONFIG_SPARC_NESTED_IRQ */
	wr	%g2, PSR_ET, %psr	! traps are off at this point, so we
	nop				! xor the bit in the new PSR to re-enable
	nop				! this is delayed-write, we need only
					! 2 nops before the call

	mov	%t_irqlvl, %o0		! set arg 1: irq level
					! nothing else for now

	call	leon_irq_dispatch	! call high-level manager
#ifndef CONFIG_SPARC_NESTED_IRQ
	 nop
#else /* CONFIG_SPARC_NESTED_IRQ */
	 or %t_psr, PSR_PIL, %t_tmp

#ifndef CONFIG_DISABLE_TASK_PREEMPTION
	call schedule
	 nop
#endif
	wr	%t_tmp, PSR_ET, %psr	! we can't be preemted here, so set PIL
					! to max and xor ET to enable
					! delayed-write; nops not needed if
                                        ! RESTORE_ALL and rtrap.S is not touced
#endif /* CONFIG_SPARC_NESTED_IRQ */

	RESTORE_ALL

