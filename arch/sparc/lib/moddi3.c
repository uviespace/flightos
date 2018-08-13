long long __divdi3 (long long n, long long d);

/**
 * @brief remainder of 64-bit signed division
 */
long long __moddi3 (long long n, long long d)
{
	return n - d * __divdi3(n, d);
}
