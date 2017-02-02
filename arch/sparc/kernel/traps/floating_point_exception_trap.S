/**
 * @file arch/sparc/kernel/traps/floating_point_exception_trap.S
 *
 * @brief this is a function that is called by a custom trap handler to handle
 *        a floating point exception
 *
 * @todo this is BCC-specific
 */



#define SAVE_ALL_HEAD \
        sethi   %hi(leonbare_trapsetup), %l4; \
        jmpl    %l4 + %lo(leonbare_trapsetup), %l6;
#define SAVE_ALL \
        SAVE_ALL_HEAD \
         nop;


#define FW_REGS_SZ     0x90 /* 36*4 */
#define SF_REGS_SZ     0x60 /* 24*4 */

/* All traps low-level code here must end with this macro. */
#define RESTORE_ALL b leonbare_trapreturn; clr %l6;



	.text
	.align 4
	.globl  floating_point_exception_trap



#define PSR_ET        0x00000020         /* enable traps field */
#define PSR_PIL_MASK  0x00000F00	/* processor interrupt level */

floating_point_exception_trap:


	mov	%l2, %l1
	add	%l2, 4, %l2

	rd	%wim, %l3	/* trap setup needs the %wim in %l3 */
	
	SAVE_ALL	/* create a trap window */

	/* re-enable traps but not interrupts (set level to max) */
	or %l0, PSR_PIL_MASK, %o0
	wr %o0, PSR_ET, %psr
	nop
	nop
	nop

	call fpe_trap
	 add	%sp, FW_REGS_SZ + 8 + SF_REGS_SZ , %o1


	RESTORE_ALL	/* also returns */





	.globl  floating_point_exception_trap_ignore

floating_point_exception_trap_ignore:

	/* see above */
	mov	%l2, %l1
	add	%l2, 4, %l2
	jmp	%l1
	rett	%l2
