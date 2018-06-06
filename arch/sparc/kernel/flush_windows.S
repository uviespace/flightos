/* SPDX-License-Identifier: GPL-2.0 */

#ifndef CONFIG_SPARC_CPU_REG_WINDOWS
#define NWINDOWS 8
#else
#define NWINDOWS CONFIG_SPARC_CPU_REG_WINDOWS
#endif /* CONFIG_SPARC_CPU_REG_WINDOWS */

#include <asm/ttable.h>
#include <stack.h>

	.align	4
	.globl	flush_windows

flush_windows:
	SAVE_ALL

	wr	%l0, PSR_ET, %psr	! delayed-write instruction
					! no nops needed, the following
					! compare-and-branch fills all 3 slots

	andcc	%l0, PSR_PS, %g0
	be	flush_user_windows
	 nop

flush_windows_kernel_all:

	.rept NWINDOWS
	save	%sp, -STACKFRAME_SZ, %sp
	.endr

	.rept NWINDOWS
	restore
	.endr

	b flush_windows_done
	 nop


flush_user_windows:

	call	flush_windows_user
	 nop


flush_windows_done:
	/* Advance over the trap instruction. */
	ld	[%sp + STACKFRAME_SZ + PT_NPC], %l1
	add	%l1, 0x4, %l2
	st	%l1, [%sp + STACKFRAME_SZ + PT_PC]
	st	%l2, [%sp + STACKFRAME_SZ + PT_NPC]

	RESTORE_ALL



	.align 4
	.globl flush_windows_user
flush_windows_user:

	ta 0	! not implemented,

