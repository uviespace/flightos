/**
 * @file    sparc/include/asm/thread.h
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
 * @brief architecture specific threads
 */


#ifndef _ARCH_SPARC_ASM_THREAD_H_
#define _ARCH_SPARC_ASM_THREAD_H_

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <stack.h>



#define NSWINS 8
struct thread_info {
	unsigned long		uwinmask;
	struct task_struct	*task;		/* main task structure */
	unsigned long		flags;		/* low level flags */
	int			cpu;		/* cpu we're on */
	int			preempt_count;	/* 0 => preemptable,
						   <0 => BUG */
	int			softirq_count;
	int			hardirq_count;

	/* bcc is stupid and does not respect the aligned attribute */
	int			unused;

	/* Context switch saved kernel state. */
	unsigned long ksp __attribute__ ((aligned (8)));
	unsigned long kpc;
	unsigned long kpsr;
	unsigned long kwim;

	/* A place to store user windows and stack pointers
	 * when the stack needs inspection.
	 */
	struct leon_reg_win	reg_window[NSWINS];	/* align for ldd! */
	unsigned long		rwbuf_stkptrs[NSWINS];
	unsigned long		w_saved;
};




#define TI_TASK	(offset_of(struct thread_info, task))
#define TI_KSP	(offset_of(struct thread_info, ksp))
#define TI_KPC  (offset_of(struct thread_info, kpc))
#define TI_KPSR (offset_of(struct thread_info, kpsr))
#define TI_KWIM (offset_of(struct thread_info, kwim))

#if 0
compile_time_assert((!(TI_KSP & 0x7UL)),
		    SPARC_THREAD_INFO_THREAD_STATE_NOT_DWORD_ALIGNED);
#endif

#endif /* _ARCH_SPARC_ASM_THREAD_H_ */
