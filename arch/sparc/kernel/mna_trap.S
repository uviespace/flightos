/* SPDX-License-Identifier: GPL-2.0 */

#include <asm/ttable.h>
#include <stack.h>

	.align	4
	.globl	mna_trap_handler

mna_trap_handler:
	SAVE_ALL

	wr	%l0, PSR_ET, %psr	! delayed-write instruction
					! no nops needed, the following
					! compare-and-branch fills all 3 slots

	andcc	%l0, PSR_PS, %g0	! check for super user flag
	be	mna_trap_user
	 nop

	ld	[%l1], %o1		! load instruction from program counter
					! (set by CPU in %l1 on trap)
	call	kernel_mna_trap
	 add	%sp, STACKFRAME_SZ, %o0	! rewind stack to offending frame

	RESTORE_ALL

mna_trap_user:
	ld	[%l1], %o1
	call	user_mna_trap
	 add	%sp, STACKFRAME_SZ, %o0

	RESTORE_ALL
