/**
 * @file    sparc/include/asm/switch_to.h
 *
 * @copyright GPLv2
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 *
 * When implementing the actual task switching segment, I came up with
 * essentially the same thing that David S. Miller did in the SPARC port
 * switch_to() macro of the Linux kernel, just less tuned, so I adapted his
 * code.
 * Hence, I decided to just follow a similar scheme to Linux for the thread
 * switching, which may make it easier to port to a new architecure in the
 * future, as it'll be possible to adapt the according macros from the Linux
 * source tree. No need to reinvent the wheel..
 *
 *
 * TODO: FPU (lazy) switching
 * TODO: CPU id (for SMP)
 */


#include <kernel/kthread.h>
#include <asm/ttable.h>

#ifndef _ARCH_SPARC_ASM_SWITCH_TO_H_
#define _ARCH_SPARC_ASM_SWITCH_TO_H_



#define prepare_arch_switch(next) do {					\
__asm__ __volatile__(							\
"save %sp, -0x40, %sp; save %sp, -0x40, %sp; save %sp, -0x40, %sp\n\t"	\
"save %sp, -0x40, %sp; save %sp, -0x40, %sp; save %sp, -0x40, %sp\n\t"	\
"save %sp, -0x40, %sp\n\t"						\
"restore; restore; restore; restore; restore; restore; restore");	\
} while(0);



/* NOTE: we don't actually require the PSR_ET toggle, but if we have
 * unaligned accesses (or access traps), it is a really good idea, or
 * we'll die...
 */

/* NOTE: this assumes we have a mixed kernel/user mapping in the MMU (if
 * we are using it), otherwise we might would not be able to load the
 * thread's data. Oh, and we'll have to do a switch user->kernel->new
 * user OR we'll run into the same issue with different user contexts */

/* curptr is %g6! */


/* so, here's what's happening
 *
 *
 * 1+2: store a new program counter (~ return address) just after the actual
 *	switch block, so the old thread will hop over the actual switching
 *	section when it is re-scheduled.
 *
 * NOTE: in the SPARC ABI the return address to the caller in %i7 is actually
 *	(return address - 0x8), and the %o[] regs become the %i[] regs on a
 *	save instruction, so we actually have to store the reference address
 *	to the jump accordingly
 *
 * 3:	store the current thread's %psr to %g4
 *
 * 4:	double-store the stack pointer (%o6) and the "skip" PC in %o7
 *	note that this requires double-word alignment of struct thread_info
 *	members KSP an KPC
 *
 * 5:	store the current thread's %wim to %g5
 *
 * 6-7:	toggle the enable traps bit in the %psr (should be off at this point!)
 *	and wait 3 cycles for the bits to settle
 *
 * 8:	double-store store the PSR in %g4 and the WIM in %g5
 *	note that this requires double-word alignment of struct thread_infio
 *	members KPSR an KWIM
 *
 * 9:	double-load KPSR + KWIM into %g4, %g5 from new thread info
 *
 * NOTE: A double load takes 2 cycles, +1 extra if the subsequent instruction
 *	 depends on the result of the load, that's why we don't set %g6 first
 *	 and use it to load  steps 10+11 form there
 *
 * 10:	set the new thread info to "curptr" (%g6) of this CPU
 *
 * 11:	set the new thread info to the global "current" set of this CPU
 *
 * 12:	set the new thread's PSR and toggle the ET bit (should be off)
 *
 * 13:	wait for the bits to settle, so the CPU puts us into the proper window
 *	before we can continue
 *
 * 14:  double-load KSP and KPC to %sp (%o6) and the "skip" PC in %o7
 *
 * 15:	set the new thread's WIM
 *
 * 16:	restore %l0 and %l1 from the memory stack, rtrap.S expects these to be
 *	l0 == t_psr, l1 == t_pc
 *
 * 17: restore the frame pointer %fp (%i6) and the return address in %i7
 *
 * 18: restore the new thread's PSR
 *
 * NOTE: we don't have to wait there, as long as we don't return immediately
 *	following the macro
 *
 * 19: jump to the actual address of the label
 *
 *
 *
 * The corresponding (approximate) c code:
 *
 *	register struct sparc_stackf *sp asm("sp");
 *	register unsigned long calladdr asm("o7");
 *	register struct thread_info *th asm("g6");
 *	register unsigned long t_psr asm("l0");
 *	register unsigned long t_pc asm("l1");
 *	register unsigned long fp asm("fp");
 *	register unsigned long ret asm("i7");
 *
 *
 *	th->kpc  = (unsigned long) &&here - 0x8;
 *	th->kpsr = get_psr();
 *	th->ksp  = (unsigned long) sp;
 *	th->kwim = get_wim();
 *
 *	put_psr(th->kpsr^0x20);
 *
 *	th = &next->thread_info;
 *	current_set[0] = th;
 *
 *	put_psr(th->kpsr^0x20);
 *	put_wim(th->kwim);
 *
 *	calladdr = th->kpc;
 *	sp = (struct sparc_stackf *) th->ksp;
 *
 *	t_psr = sp->locals[0];
 *	t_pc = sp->locals[1];
 *
 *	fp = (unsigned long) sp->fp;
 *	ret = sp->callers_pc;
 *
 *	put_psr(th->kpsr);
 *
 *	__asm__ __volatile__(
 *		"jmpl	%%o7 + 0x8, %%g0\n\t"
 *		"nop\n\t"
 *		::: "%o7", "memory");
 *	here:
 *		(void) 0;
 */


#define switch_to(next)	do {					\
	__asm__ __volatile__(					\
	"sethi	%%hi(here - 0x8), %%o7\n\t"			\
	"or	%%o7, %%lo(here - 0x8), %%o7\n\t"		\
	"rd	%%psr, %%g4\n\t"				\
	"std	%%sp, [%%g6 + %2]\n\t"				\
	"rd	%%wim, %%g5\n\t"				\
	"wr	%%g4, 0x20, %%psr\n\t"				\
	"nop; nop; nop\n\t"					\
	"std	%%g4, [%%g6 + %4]\n\t"				\
	"ldd	[%1 + %4], %%g4\n\t"				\
	"mov	%1, %%g6\n\t"					\
	"st	%1, [%0]\n\t"					\
	"wr	%%g4, 0x20, %%psr\n\t"				\
	"nop; nop; nop\n\t"					\
	"ldd	[%%g6 + %2], %%sp\n\t"				\
	"wr	%%g5, 0x0, %%wim\n\t"				\
	"ldd	[%%sp + 0x00], %%l0\n\t"			\
	"ldd	[%%sp + 0x38], %%i6\n\t"			\
	"wr	%%g4, 0x0, %%psr\n\t"				\
	"nop; nop\n\t"						\
	"jmpl %%o7 + 0x8, %%g0\n\t"				\
	" nop\n\t"						\
	"here:\n\t"						\
	:							\
	: "r" (&(current_set[smp_cpu_id()])),			\
	  "r" (&(next->thread_info)),				\
	  "i" (TI_KSP),						\
	  "i" (TI_KPC),						\
	  "i" (TI_KPSR)						\
	:       "g1", "g2", "g3", "g4", "g5",       "g7",	\
	  "l0", "l1",       "l3", "l4", "l5", "l6", "l7",	\
	  "i0", "i1", "i2", "i3", "i4", "i5",			\
	  "o0", "o1", "o2", "o3",                   "o7");	\
} while(0);




#endif /* _ARCH_SPARC_ASM_SWITCH_TO_H_ */

