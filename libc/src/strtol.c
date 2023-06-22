#include <ctype.h>
#include <limits.h>

#ifndef __WORDSIZE
#define __WORDSIZE (__SIZEOF_LONG__ * 8)
#endif

#ifndef BITS_PER_LONG
#define BITS_PER_LONG __WORDSIZE
#endif

#ifndef BITS_PER_LONG_LONG
#define BITS_PER_LONG_LONG (__WORDSIZE * 2)
#endif

/* XXX this only handles 32 bit words ... */
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
 * @brief convert a string to a long integer
 *
 * @param nptr	the pointer to a string (possibly) representing a number
 * @param endptr if not NULL, stores the pointer to the first character not part
 *		 of the number
 * @param base  the base of the number string
 *
 * @return the converted number
 *
 * @note A base of 0 defaults to 10 unless the first character after
 *	 ' ', '+' or '-' is a '0', which will default it to base 8, unless
 *	 it is followed by 'x' or 'X'  which defaults to base 16.
 *	 If a number is not preceeded by a sign, it is assumed to be a unsigned
 *	 type and may therefore assume the size of ULONG_MAX.
 *	 If no number has been found, the function will return 0 and
 *	 nptr == endptr
 *
 * @note if the value would over-/underflow, LONG_MAX/MIN is returned
 */

long int strtol(const char *nptr, char **endptr, int base)
{
	int neg = 0;

	unsigned long res = 0;
	unsigned long clamp = (unsigned long) ULONG_MAX;



	if (endptr)
		(*endptr) = (char *) nptr;

	for (; isspace((int) (*nptr)); nptr++);


	if ((*nptr) == '-') {
		nptr++;
		neg = 1;
		clamp = -(unsigned long) LONG_MIN;
	} else if ((*nptr) == '+') {
		clamp =  (unsigned long) LONG_MAX;
		nptr++;
	}

	if (!base || base == 16) {
		if ((*nptr) == '0') {
			switch (nptr[1]) {
			case 'x':
			case 'X':
				nptr += 2;
				base = 16;
				break;
			default:
				nptr++;
				base = 8;
				break;
			}
		} else {
			/* default */
			base = 10;
		}
	}


	/* Now iterate over the string and add up the digits. We convert
	 * any A-Z to a-z (offset == 32 in ASCII) to check if the string
	 * contains letters representing values (A=10...Z=35). We abort if
	 * the computed digit value exceeds the given base or on overflow.
	 */

	while (1) {
		unsigned int c = (*nptr);
		unsigned int l = c | 0x20;
		int val;

		if (isdigit(c))
			val = c - '0';
		else if (islower(l))
		       val = l - 'a' + 10;
		else
			break;

		if (val >= base)
			break;

		/* Check for overflow only if result is approximately within
		 * range of it, given the base. If we overflow, set result
		 * to clamp (LONG_MAX or LONG_MIN)
		 */

		if (res & (~0UL << (BITS_PER_LONG - fls(base)))) {
	               if (res > (clamp - val) / base) {
			       res = clamp;
			       break;
		       }
		}

		res = res * base + val;
		nptr++;
	}


	/* update endptr if a value was found */
	if (res)
		if (endptr)
			(*endptr) = (char *) nptr;


	if (neg)
		return - (long int) res;

	return (long int) res;
}
