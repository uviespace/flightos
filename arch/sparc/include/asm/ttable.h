/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SPARC_TTABLE_H
#define _SPARC_TTABLE_H

#define SPARC_TRAP_TFLT    0x1          /* Text fault */
#define SPARC_TRAP_II      0x2          /* Illegal Instruction */
#define SPARC_TRAP_PI      0x3          /* Privileged Instruction */
#define SPARC_TRAP_FPD     0x4          /* Floating Point Disabled */
#define SPARC_TRAP_WOVF    0x5          /* Window Overflow */
#define SPARC_TRAP_WUNF    0x6          /* Window Underflow */
#define SPARC_TRAP_MNA     0x7          /* Memory Address Unaligned */
#define SPARC_TRAP_FPE     0x8          /* Floating Point Exception */
#define SPARC_TRAP_DFLT    0x9          /* Data Fault */
#define SPARC_TRAP_TOF     0xa          /* Tag Overflow */
#define SPARC_TRAP_WPDT    0xb          /* Watchpoint Detected */
#define SPARC_TRAP_IRQ1    0x11         /* IRQ level 1 */
#define SPARC_TRAP_IRQ2    0x12         /* IRQ level 2 */
#define SPARC_TRAP_IRQ3    0x13         /* IRQ level 3 */
#define SPARC_TRAP_IRQ4    0x14         /* IRQ level 4 */
#define SPARC_TRAP_IRQ5    0x15         /* IRQ level 5 */
#define SPARC_TRAP_IRQ6    0x16         /* IRQ level 6 */
#define SPARC_TRAP_IRQ7    0x17         /* IRQ level 7 */
#define SPARC_TRAP_IRQ8    0x18         /* IRQ level 8 */
#define SPARC_TRAP_IRQ9    0x19         /* IRQ level 9 */
#define SPARC_TRAP_IRQ10   0x1a         /* IRQ level 10 */
#define SPARC_TRAP_IRQ11   0x1b         /* IRQ level 11 */
#define SPARC_TRAP_IRQ12   0x1c         /* IRQ level 12 */
#define SPARC_TRAP_IRQ13   0x1d         /* IRQ level 13 */
#define SPARC_TRAP_IRQ14   0x1e         /* IRQ level 14 */
#define SPARC_TRAP_IRQ15   0x1f         /* IRQ level 15 Non-maskable */
#define SPARC_TRAP_RAC     0x20         /* Register Access Error ??? */
#define SPARC_TRAP_IAC     0x21         /* Instruction Access Error */
#define SPARC_TRAP_CPDIS   0x24         /* Co-Processor Disabled */
#define SPARC_TRAP_BADFL   0x25         /* Unimplemented Flush Instruction */
#define SPARC_TRAP_CPEX    0x28         /* Co-Processor Exception */
#define SPARC_TRAP_DACC    0x29         /* Data Access Error */
#define SPARC_TRAP_DIVZ    0x2a         /* Divide By Zero */
#define SPARC_TRAP_DSTORE  0x2b         /* Data Store Error (?) */
#define SPARC_TRAP_DMM     0x2c         /* Data Access MMU Miss (?) */
#define SPARC_TRAP_IMM     0x3c         /* Instruction Access MMU Miss (?) */


#define PSR_PS	0x00000040         /* previous privilege level */
#define PSR_S	0x00000080         /* enable supervisor */

#define PSR_ET	0x00000020	   /* enable traps */
#define PSR_EF	0x00001000         /* enable FPU */

#define PSR_PIL	0x00000f00	/* processor interrupt level */
#define PSR_PIL_SHIFT	 8	/* PIL field shift */


#define TTBL_MASK	0xff0	/* trap type mask from tbr */
#define TTBL_SHIFTLEFT 	    4	/* shift to get a tbr value */

#define TRAP_ENTRY(type, label) rd %psr, %l0; b label; rd %wim, %l3; nop;

#define SRMMU_TFAULT rd %psr, %l0; rd %wim, %l3; b srmmu_fault; mov 1, %l7;
#define SRMMU_DFAULT rd %psr, %l0; rd %wim, %l3; b srmmu_fault; mov 0, %l7;



#define WINDOW_OFLOW \
        rd %psr, %l0; rd %wim, %l3; b __wim_overflow; andcc %l0, PSR_PS, %g0;

#define WINDOW_UFLOW \
        rd %psr, %l0; rd %wim, %l3; b __wim_underflow; andcc %l0, PSR_PS, %g0;


/* either this */
#define TRAP_EXCEPTION \
	 rd %tbr, %l3; rd %psr, %l0; ba	__exception_entry; and	%l3, TTBL_MASK,	%l4;
/* or this one */
#define TRAP_BAD(num) \
        rd %psr, %l0; mov num, %l7; b bad_trap_handler; rd %wim, %l3;


#define TRAP_INTERRUPT(level) \
        rd %psr, %l0; mov level, %l7;  b __interrupt_entry; rd %wim, %l3;

/* system calls software trap */
#define SYSCALL_TRAP \
        sethi %hi(syscall_tbl), %l7; \
        or %l7, %lo(syscall_tbl), %l7; \
        b syscall_trap; \
        rd %psr, %l0;


/* This is the same convention as in leonbare and linux */
/* All trap entry points _must_ begin with this macro or else you
 * lose.  It makes sure the kernel has a proper window so that
 * c-code can be called.
 */
#define SAVE_ALL_HEAD \
	sethi	%hi(trap_setup), %l4; \
	jmpl	%l4 + %lo(trap_setup), %l6;
#define SAVE_ALL \
	SAVE_ALL_HEAD \
	 nop;

/* All traps low-level code here must end with this macro. */
#define RESTORE_ALL b ret_trap_entry; clr %l6;


#endif /* _SPARC_TTABLE_H */
