/**
 * @file lib/libc_bitops.c
 *
 * @ingroup libc
 * @defgroup functions expected by the compiler
 *
 * @note these can be overridden by linking architecture-specific implementation
 *
 * @note add as needed
 *
 * @see https://gcc.gnu.org/onlinedocs/gccint/Integer-library-routines.html
 *
 */


/**
 * @brief locates the most significant bit set in a word
 * @note taken from linux: include/asm-generic/bitops/fls.h
 */

static int fls(int x)
{
	int r = 32;

	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

/**
 * @brief returns the number of leading 0-bits in a,
 *	  starting at the most significant bit position.
 *	  If a is zero, the result is undefined.
 */

int __attribute__((weak)) __clzsi2(unsigned int a)
{
	return 32 - fls(a);
}


int __attribute__((weak)) __clzdi2(long val)
{
	return 32 - fls((int)val);
}
