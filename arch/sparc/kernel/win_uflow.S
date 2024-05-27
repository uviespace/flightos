/* SPDX-License-Identifier: GPL-2.0 */

#include <asm/win.h>

/* As we enter this trap handler, we expect the following setup:
 * PSR in l0
 * PC  in l1 (set by CPU)
 * NPC in l2 (set by CPU)
 * WIM in l3
 * PSR_PS bit condition code set by trap (i.e. andcc %l0, PSR_PS, %g0)
 *
 * We will use %l4 and %l5 as temporary registers.
 */

#ifndef CONFIG_SPARC_CPU_REG_WINDOWS
#define NWINDOWS 8
#else
#define NWINDOWS CONFIG_SPARC_CPU_REG_WINDOWS
#endif /* CONFIG_SPARC_CPU_REG_WINDOWS */

#if (NWINDOWS < 3)
#error "This code will not work on CPUs with < 3 register windows implemented."
#endif

	.text
	.align	4
	.global	__wim_underflow

__wim_underflow:
	/* A trap always decrements the CWP, so we are now 2 windows away from
	 * the window we want to get into.
	 *
	 * First, we rotate the window invalid mask to the left by one and
	 * to the right by (NWINDOWS - 1), then OR the results (which is done
	 * implicitly in the WRPSR instruction), so our WIM rotates modulo
	 * NWINDOWS just like the CWP in the PSR
	 */

        sll     %l3, 1, %l4
        srl     %l3, (NWINDOWS - 1), %l5

        wr      %l4, %l5, %wim	! delayed-write instruction
	nop			! need 2 delays, restore modifies CWP
	nop			! 3rd cycle is filled by branch instruction

	/* At this point, we proceed depending on the state of the previous
	 * supervisor (PS) bit. The condition code has been set by the trap
	 * entry.
	 */
	be win_trap_from_user
	 restore		! first restore, we'll need another to restore
				! to the target window (user->caller)
win_trap_from_superuser:
	restore			! second restore

win_trap_finish:
	LOAD_WINDOW(sp)		! load stack window from memory

	save			! return to trap window
        save

        wr      %l0, %g0, %psr	! restore PSR, delayed-write instruction
	nop
	nop
	nop
	/* load %pc from %l1 and  return to execute the trapped save instruction
	 * at %npc
	 */
        jmp     %l1
        rett    %l2



/* XXX: basic structure, add more checks as needed */
win_trap_from_user:
	restore			! second restore

	/* users cannot be trusted, so we'll perform a basic sanity check for
	 * doubleword-alignment of the user's stack pointer
	 */
	! b win_trap_stack_check
	andcc %sp, (16 - 1), %g0

win_trap_stack_check:

	/* evaluate check 1: stack alignment */
	bne win_trap_user_stack_error
	 nop

#ifdef CONFIG_MMU
	/* XXX: add check 2: stack memory address in user-spaced mapped VMA? */
#endif
	/* XXX: maybe check 3: stack memory address in user-allocated page?
	 *	(e.g. to catch cross-process stack jumps)
	 */

	/* all checks passed, finish up */
	b win_trap_finish
	 nop

/* XXX: do something in case of user stack error, e.g. terminate process */
win_trap_user_stack_error:
	ta 0	! trap again to halt
