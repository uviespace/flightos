/* SPDX-License-Identifier: GPL-2.0 */

/* XXX: there is no handling of trapping from user space at this time,
 *	we'll add that when we implement threads
 *
 * what we probably will do:
 * - maintain a list of thread info pointers (one per CPU)
 * - identify the CPU, fetch pointer to current thread's structure from list
 *   and store the pointer of the thread structure in one of %g6-%g7 (these
 *   are not used by the compiler), so we can track the info across windows
 * - 
 */

#include <asm/win.h>

/* As we enter this trap handler, we expect the following setup:
 * PSR in l0
 * PC  in l1 (set by CPU)
 * NPC in l2 (set by CPU)
 * WIM in l3
 * PSR_PS bit condition code set by trap (i.e. andcc %l0, PSR_PS, %g0)
 *
 * We will use %l4 as temporary local registers.
 * We will store the next WIM in the %g5, since we need it across windows
 *  - the previous %g5 will be stored in %l5 temporarily.
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
	.global	__wim_overflow

__wim_overflow:

	/* we use %g5 to store the new WIM, store original in %l5 */
	mov	%g5, %l5

	/* First, we rotate the window invalid mask to the right by one and
	 * to the left by (NWINDOWS - 1), then OR the results (which is done
	 * implicitly in the WRPSR instruction), so our WIM rotates modulo
	 * NWINDOWS just like the CWP in the PSR
	 */

        srl     %l3, 1, %l4
        sll     %l3, (NWINDOWS - 1), %g5
	or	%l4, %g5, %g5	! store new WIM globally

	/* At this point, we proceed depending on the state of the previous
	 * supervisor (PS) bit. The condition code has been set by the trap
	 * entry.
	 */
	/* XXX: not implemented */
	be,a win_trap_from_user	! trapped from user mode
	 save			! annulled if branch not taken

	/* XXX: user window mask check and handling goes here */

	/* Otherwise, change to the window which has to be saved and store it
	 * to memory
	 */
	save			! move to window
	wr	%g5, %g0, %wim 	! delayed-branch instruction
				! delay cycles are covered in store block

	STORE_WINDOW(sp)	! store to memory

	restore			! return to trap window

	mov 	%l5, %g5	! restore original %g5

        wr      %l0, %g0, %psr	! restore PSR, delayed-write instruction
	nop
	nop
	nop
	/* load %pc from %l1 and  return to execute the trapped save instruction
	 * at %npc
	 */
        jmp     %l1
        rett    %l2




win_trap_from_user:
	ta 0	! not implemented, halt CPU
