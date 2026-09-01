/* SPDX-License-Identifier: GPL-2.0 */

/**
 * @file arch/sparc/kernel/irqtrap.S
 * @ingroup interrupts
 *
 * @brief SPARC interrupt (IRQ) trap entry
 *
 * This is the low-level trap entry for interrupts, reached from the trap
 * table (ttable.S) through the TRAP_INTERRUPT(level) macro. It is the
 * architecture-side counterpart of the high-level interrupt handling in
 * arch/sparc/kernel/irq.c and the dispatcher of the generic IRQ interface in
 * kernel/irq.c.
 *
 * ## Entry context
 *
 * The trap table sets, before branching here, the per-trap registers used
 * throughout this file (see asm/ttable.h):
 *
 *   - %l0 (t_psr)    : PSR at the time of the trap
 *   - %l3 (t_wim)    : WIM at the time of the trap
 *   - %l7 (t_irqlvl) : the interrupt level that was trapped (set by
 *                      TRAP_INTERRUPT(level); also passed on in %o0)
 *
 * Interrupt levels 1-14 are routed here; level 15 (NMI) is handled
 * separately by nmi_entry (TRAP_ENTRY(15, nmi_entry)).
 *
 * ## Routing in the IRQ subsystem
 *
 *   - ttable.S -> TRAP_INTERRUPT(level) -> __interrupt_entry
 *   - SAVE_ALL -> trap_setup (etrap.S) builds the register window and
 *     pt_regs frame; on return the PSR PIL is re-programmed:
 *       + with CONFIG_SPARC_NESTED_IRQ the PIL is set to the trapped IRL, so
 *         higher-level interrupts may preempt the handler;
 *       + otherwise PIL is set to maximum, disabling all nesting.
 *     ET is re-enabled in the same wr (traps were off on entry, so the bit is
 *     xored into the new PSR).
 *   - leon_irq_dispatch(irq) (arch/sparc/kernel/irq.c) runs the registered
 *     handlers for the level. With CONFIG_ARCH_CUSTOM_BOOT_CODE this call is
 *     made here directly; otherwise BCC's libgloss catch_interrupt() is used
 *     instead of this trap entry.
 *   - With CONFIG_SPARC_NESTED_IRQ and task preemption enabled, the entry
 *     sets PIL to maximum (write is delayed until after schedule()) and calls
 *     schedule() before returning.
 *   - RESTORE_ALL -> ret_trap_entry (rtrap.S) unwinds the frame and returns
 *     to the interrupted code.
 *
 * ## Cycle budget
 *
 * The comment block below documents the measured/estimated cycle cost of the
 * trap + jump + SAVE_ALL + leon_irq_dispatch + schedule + RESTORE_ALL path
 * (worst case with window overflow ~195 cycles before the ISR body).
 *
 * Note: the Doxygen build configuration in Documentation/doxygen/Doxyfile
 * does not currently map/source *.S files, so this file is not picked up by
 * the documentation build (needs review).
 */

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

	sll     %t_irqlvl, PSR_PIL_SHIFT, %l5	! shift into place
	andn    %t_psr, PSR_PIL, %l6		! clear field
	or      %l5, %l6, %l6			! set new level

#else /* !(CONFIG_SPARC_NESTED_IRQ)
	/* ... otherwise we just set it to max */

	or      %t_psr, PSR_PIL, %l6

#endif /* CONFIG_SPARC_NESTED_IRQ */
	wr	%l6, PSR_ET, %psr	! traps are off at this point, so we
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
