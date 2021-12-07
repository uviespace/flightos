#include <compiler.h>

__diag_push()
__diag_ignore(GCC, 7, "-Wlong-long", "we need this in vsnprintf()")

unsigned long long __udivdi3 (unsigned long long n, unsigned long long d);

/**
 * @brief remainder of 64-bit unsigned division
 */
unsigned long long __umoddi3 (unsigned long long n, unsigned long long d)
{
	return n - d * __udivdi3(n, d);
}
__diag_pop()
