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
#include <uapi/asi.h>


#ifndef CONFIG_SPARC_CPU_REG_WINDOWS
#define NWINDOWS 8
#else
#define NWINDOWS CONFIG_SPARC_CPU_REG_WINDOWS
#endif /* CONFIG_SPARC_CPU_REG_WINDOWS */


/* Registers to not touch at all. */

#define t_psr     l0
#define t_pc      l1
#define t_npc     l2
#define t_wim     l3
#define twin_tmp1 l4
#define glob_tmp  g4
#define curptr    g6


	.globl	ret_trap_entry
ret_trap_entry:


ret_trap_kernel:

	/* Will the rett land us in the invalid window? */
	mov	2, %l5
	sll	%l5, %t_psr, %l5
	srl	%l5, NWINDOWS, %l6
	or	%l5, %l6, %l5
	rd	%wim, %l6
	andcc	%l6, %l5, %g0
	be	1f		! Nope, just return from the trap
	 sll	%l6, 0x1, %l5

	/* window would be invalid, apply rotated mask (same as in win_uflow) */
	srl	%l6, (NWINDOWS - 1),  %l6
	or	%l5, %l6, %l5

	/* no need to mask unused bits here, the CPU ignores any unimplemented
	 * windows
	*/
	wr	%l5, 0x0, %wim

	LOAD_PT_ALL(sp, t_psr, t_pc, t_npc, l5)

	restore
	LOAD_WINDOW(sp)
	b	2f
	 save

	/* Reload the entire frame in case this is from a
	 * kernel system call or whatever...
	 */
1:
	LOAD_PT_ALL(sp, t_psr, t_pc, t_npc, l5)
2:

#if defined(LEON3FT_ERRATUM_TN0018)
1:
        lda     [%g0] ASI_LEON_CACHEREGS, %l4
        srl     %l4, ASI_LEON_CACHEREGS_IP_BIT_SHIFT, %l5
        andcc   %l5, 1, %g0
        bne     1b
         andn   %l4, ASI_LEON_CACHEREGS_ICS, %l5
#endif	/* LEON3FT_ERRATUM_TN0018 */

	wr	%t_psr, %g0, %psr ! restore PSR, delayed-write instruction
	nop
	nop
	nop

#if defined(LEON3FT_ERRATUM_TN0018)
	.align  0x20				! align the sta for performance
	sta     %l5, [%g0] ASI_LEON_CACHEREGS	! disable icache
	nop					! delay for sta to have effect on rett
	or      %l1, %l1, %l1			! delay + catch rf parity error on l1
	or      %l2, %l2, %l2			! delay + catch rf parity error on l2
	sta     %l4, [%g0] ASI_LEON_CACHEREGS	! re-enable icache after rett
	nop					! delay ensures insn after gets cached
#endif	/* LEON3FT_ERRATUM_TN0018 */

	jmp	%t_pc
	rett	%t_npc
