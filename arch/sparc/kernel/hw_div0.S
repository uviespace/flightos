/* SPDX-License-Identifier: GPL-2.0 */

#include <asm/ttable.h>
#include <stack.h>

	.align	4
	.globl	hw_div0_trap_handler

hw_div0_trap_handler:
	SAVE_ALL

	wr	%l0, PSR_ET, %psr	! delayed-write instruction
	nop
	nop
	nop

	call	hw_div0
	 nop

	RESTORE_ALL
