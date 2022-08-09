/**
 *
 * This is our basic SPARC v8 CRT0
 *
 * note we only do argc/argv as per POSIX.1, i.e. no envp, as we don't
 * use shells
 *
 */

#include <stdlib.h>
#include <limits.h>


__asm(
      "	.text			\n\t"
      "	.align 4		\n\t"
      "	.global	_start		\n\t"
      "_start:			\n\t"
      "				\n\t"
      " save	%sp, -0x40, %sp		! OS-provided stack		\n\t"
      "	ld	[%sp + 64], %o0		! argc				\n\t"
      "	add	%sp, 68, %o1		! argv				\n\t"
      "	andn	%sp, 7,	%sp		! align				\n\t"
      "	call	main							\n\t"
      "	 sub	%sp, 24, %sp		! full 92 byte stack frame	\n\t"
      " mov	%o0, %i0		! retval			\n\t"
      " ret								\n\t"
      " restore								\n\t"
     );

/**
 *  XXX: at some point we will need some kind of explicit exit() call so we can
 *  inform the OS that the program returned from main()
 */
