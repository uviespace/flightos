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

#ifndef CONFIG_TASK_PREEMPTION_DISABLE
	call schedule
	 nop
#endif
	wr	%t_tmp, PSR_ET, %psr	! we can't be preemted here, so set PIL
					! to max and xor ET to enable
					! delayed-write; nops not needed if
                                        ! RESTORE_ALL and rtrap.S is not touced
#endif /* CONFIG_SPARC_NESTED_IRQ */

	RESTORE_ALL

