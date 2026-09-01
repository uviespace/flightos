/**
 * @file arch/sparc/lib/umoddi3.c
 *
 * @brief 64-bit unsigned modulo (compiler runtime)
 */

unsigned long long __udivdi3 (unsigned long long n, unsigned long long d);

/**
 * @brief remainder of 64-bit unsigned division
 *
 * @param n the dividend
 * @param d the divisor
 *
 * @return the remainder of n / d
 */
unsigned long long __umoddi3 (unsigned long long n, unsigned long long d)
{
	return n - d * __udivdi3(n, d);
}
