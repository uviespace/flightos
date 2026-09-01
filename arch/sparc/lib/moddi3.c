/**
 * @file arch/sparc/lib/moddi3.c
 *
 * @brief 64-bit signed modulo (compiler runtime)
 */

long long __divdi3 (long long n, long long d);

/**
 * @brief remainder of 64-bit signed division
 *
 * @param n the dividend
 * @param d the divisor
 *
 * @return the remainder of n / d
 */
long long __moddi3 (long long n, long long d)
{
	return n - d * __divdi3(n, d);
}
