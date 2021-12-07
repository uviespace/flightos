#include "byteorder.h"
#include <compiler.h>


/* Someboedy is doing some uncontrolled byte swapping it seems.
 * I want this to just run right now, so I'll provide the function even though
 * there is most likely bad code somewhere
 */

int printf(const char *fmt, ...);

int32_t __bswapsi2(int32_t a)
{
__diag_push()
__diag_ignore(GCC, 7, "-Wframe-address", "we want this to trace the culprit")
	printf("WARNING: you are likely doing some uncontrolled byteswapping in call from address "
	       "%p\n",  __builtin_return_address((1)));
__diag_pop()

	return (int32_t) __bswap_constant_32((uint32_t) a);
}
