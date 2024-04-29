/* SPDX-License-Identifier: GPL-2.0 */

#include <asm/ttable.h>
#include <stack.h>

	.align	4
	.globl	write_buffer_error_trap_handler

write_buffer_error_trap_handler:
	SAVE_ALL

	wr	%l0, PSR_ET, %psr	! delayed-write instruction
	nop
	nop
	nop

	call	kernel_write_buffer_error
	 add	%sp, STACKFRAME_SZ, %o0	! rewind stack to offending frame

	RESTORE_ALL
