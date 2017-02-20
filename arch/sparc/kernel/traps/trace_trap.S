/**
 * @file arch/sparc/kernel/traps/trace_trap.S
 * @ingroup stack_trace

 * @brief this is a function that is called by a custom trap handler to perform
 *        a backtrace
 */

	.text                                                                   
	.align 4                                                                
	.globl  trace_trap

#define PSR_ET      0x00000020         /* enable traps field         */

trace_trap:
	/* re-enable traps */
	rd %psr, %l0
	wr %l0, PSR_ET,%psr
	nop
	nop
	nop

	/* %sp of trapped function */
	mov %fp, %o0

	/* %pc of trapped function is stored in %l1 */
	mov %l1, %o1
	call trace
	 nop
	call die
	 nop
/* if we wanted to return from this: */
#	jmp  %l1
#	rett %l2
