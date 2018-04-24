
#ifndef CONFIG_SPARC_CPU_REG_WINDOWS
#define NWINDOWS 8
#else
#define NWINDOWS CONFIG_SPARC_CPU_REG_WINDOWS
#endif /* CONFIG_SPARC_CPU_REG_WINDOWS */


#ifndef CONFIG_SPARC_FP_START
#define FP_INITIAL	0x40100000
#else
#define FP_INITIAL CONFIG_SPARC_FP_START
#endif /* CONFIG_SPARC_FP_START */



	.global kernel_entry

kernel_entry:

	/* First, we set the address of our trap table.	The base address is
	 * stored in bits 31:12 of the trap base register. The remaining fields
	 * are not affected by a WRTBR. In any case, the trap table must be
	 * aligned to a 4k boundary, so the bits 11:0 must be zero anyways or
	 * we are surely boned.
	 */
        sethi   %hi(trapbase), %g1
        or      %g1, %lo(trapbase), %g1
        mov     %g1, %tbr	! delayed-write instruction
	/* WRTBR is a delayed-write instruction (0-3 cycles, impl.dependent),
	 * but we won't access it in the next cycles, so we may continue without
	 * inserting (3) nops.
	 */


	/* Now we initialise the window invalid mask to detect under/overflows.
	 * We set the highest-value window to be invalid.
	 */

        mov     (1 << (NWINDOWS - 1)), %g1
        mov     %g1, %wim	! delayed-write instruction


	/* Initially, we want to start out in the window just below the one
	 * marked invalid, so we can SAVE up to NWINDOWS - 2 times.
	 * This is because any subsequent function call (resulting in increasing
	 * call depth) will require a SAVE instruction, which will in
	 * turn decrement the current window pointer in the PSR. If the CWP
	 * reaches the invalid window while increasing the call depth, an
	 * overflow trap will be generated. If a RESTORE reaches the invalid
	 * window due to decreasing call depth and incrementing CWP value,
	 * a underflow trap will occur.
	 *
	 * See the SPARCv8 User Manual pp.28 for the other set bits
	 */

        sethi   %hi(0x00001fc0), %g1
        or      %g1, %lo(0x00001fc0 | (NWINDOWS - 2)), %g1
        mov     %g1, %psr	! delayed-write instruction


	/* Prepare a C call-stack. Minimum size is 96 bytes, however older
	 * compilers may reserve 104. We want make sure to support the MPPB's
	 * GCC 3.x, so we'll go with the slightly larger window size here.
	 */

        sethi   %hi(FP_INITIAL), %g2
        or      %g2, %lo(FP_INITIAL), %g2
        mov     %g2, %fp
        sub     %fp, 104, %sp	! create a new frame

        /* We have a stack now, so we can start handling traps. */
	or	%g1, 0x00000020, %g1
        mov     %g1, %psr ! delayed-write instruction

	call do_basic_setup
	 nop

	call kernel_main
	 nop
