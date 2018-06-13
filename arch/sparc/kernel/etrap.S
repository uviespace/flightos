/* SPDX-License-Identifier: GPL-2.0 */
/*
 * etrap.S: Sparc trap window preparation for entry into the
 *          Linux kernel.
 *
 * Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
		 Armin Luntzer <armin.luntzer@univie.ac.at>
 */

#include <asm/ttable.h>
#include <stack.h>


#ifndef CONFIG_SPARC_CPU_REG_WINDOWS
#define NWINDOWS 8
#else
#define NWINDOWS CONFIG_SPARC_CPU_REG_WINDOWS
#endif /* CONFIG_SPARC_CPU_REG_WINDOWS */


/* Registers to not touch at all. */
#define t_psr        l0 /* Set by caller */
#define t_pc         l1 /* Set by caller */
#define t_npc        l2 /* Set by caller */
#define t_wim        l3 /* Set by caller */
#define t_twinmask   l4 /* Set at beginning of this entry routine. */
#define t_kstack     l5 /* Set right before pt_regs frame is built */
#define t_retpc      l6 /* If you change this, change winmacro.h header file */
#define t_systable   l7 /* Never touch this, could be the syscall table ptr. */
#define curptr       g6 /* Set after pt_regs frame is built */

	.text
	.align 4

	.globl	trap_setup
trap_setup:
	/* Calculate mask of trap window.  See if from user
	 * or kernel and branch conditionally.
	 */
	andcc	%t_psr, PSR_PS, %g0		 ! fromsupv_p = (psr & PSR_PS)
	mov	1, %t_twinmask
	be	trap_setup_from_user		 ! nope, from user mode
	 sll	%t_twinmask, %t_psr, %t_twinmask ! t_twinmask = (1 << psr)

	/* From kernel, allocate more kernel stack and
	 * build a pt_regs trap frame.
	 */
	sub	%fp, (STACKFRAME_SZ + PTREG_SZ), %t_kstack
	STORE_PT_ALL(t_kstack, t_psr, t_pc, t_npc, g2)

	/* See if we hit a window marked invalid, i.e. whether we have
	 * overflowed/spilled as part of this trap
	 */
	andcc	%t_twinmask, %t_wim, %g0
	bne	trap_setup_kernel_win_oflow	! window overflow 
	 nop

	/* Trap from kernel with a window available.
	 * Just do it...
	 */
	jmpl	%t_retpc + 0x8, %g0	! return to caller
	 mov	%t_kstack, %sp		! jump onto new stack


trap_setup_kernel_win_oflow:
	/* here we check whether there are user windows. this is not relevant
	 * until we do a form of lazy context switching between kernel and
	 * userspace */
!	ld	[%curptr + TI_UWINMASK], %g1
!	orcc	%g0, %g1, %g0
!	bne	trap_setup_user_spill	! there are some user windows, yuck


	/* We hit the invalid window (overflow), but there are no user windows,
	 * so we do mostly the same thing as in __wim_overflow
	 */

	/*
	 * First, we rotate the window invalid mask to the right by one and
	 * to the left by (NWINDOWS - 1), then OR the results (which is done
	 * implicitly in the WRPSR instruction), so our WIM rotates modulo
	 * NWINDOWS just like the CWP in the PSR
	 */

	srl	%t_wim, 0x1, %g2
	sll	%t_wim, (NWINDOWS - 1), %t_wim
	or	%t_wim, %g2, %g2	! store new WIM globally
	!and	%g2, ((1 << NWINDOWS) - 1), %g2 !not needed, we can always write set bits of unused windows to the wim


	save			! move to window
	wr	%g2, %g0, %wim 	! delayed-branch instruction
				! delay cycles are covered in store block

	STORE_WINDOW(sp)	! store kernel window to memory stack

	restore			! return to trap window

	jmpl	%t_retpc + 0x8, %g0	! return to caller
	 mov	%t_kstack, %sp		! and onto new kernel stack


trap_setup_user_spill:
	ta 0	! not implemented, same stuff as in win_oflow.S
trap_setup_from_user:
	ta 0	! not implemented
