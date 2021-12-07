#include <stdint.h>
#include <stddef.h>

#ifndef USE_OPTIMISATIONS

/**
 * @brief copy a memory area src that may overlap with area dest
 *
 * @param dest the destination memory area
 * @param src the source memory area
 * @param n the number of bytes to copy
 *
 * @returns a pointer to dest
 */

void *memmove(void *dest, const void *src, size_t n)
{
	char *d;

	const char *s;

	if (dest <= src) {

		d = dest;
		s = src;

		while (n--) {
			(*d) = (*s);
			d++;
			s++;
		}

	} else {

		d = dest;
		d += n;

		s = src;
		s += n;

		while (n--) {
			d--;
			s--;
			(*d) = (*s);
		}

	}
	return dest;
}


#else /* USE_OPTIMISATIONS */

static void memmove_fwd(char *d, const char *s, size_t n);
static void memmove_bwd(char *d, const char *s, size_t n);

/**
 * @brief copy a memory area src that may overlap with area dest
 *
 * @param dest the destination memory area
 * @param src the source memory area
 * @param n the number of bytes to copy
 *
 * @note this function provides some optimisation by using word-size load/stores
 *	 when possible
 *
 * @returns a pointer to dest
 */

void *memmove(void *dest, const void *src, size_t n)
{
	if (!n || dest == src)
		return dest;

	if ((uintptr_t) dest < (uintptr_t) src)
		memmove_fwd(dest, src, n);
	else
		memmove_bwd(dest, src, n);

	return dest;
}

static void memmove_fwd(char *d, const char *s, size_t n)
{
	size_t c;

	if (((uintptr_t) s | (uintptr_t) d) & (sizeof(int) - 1)) {

		/* if the addresses are not aligned to word boundaries but
		 * can be aligned by advancing a few bytes, copy those bytes,
		 * then move on to more efficient word-sized copy below.
		 * If both addresses are always unaligned, just do a
		 * byte-wise copy
		 */

		if (n < sizeof(int)) {
			c = n;	/* there isn't much to copy ... */
		} else if (((uintptr_t) s ^ (uintptr_t) d) & (sizeof(int) - 1)) {
			c = n; /* one pointer is always unaligned */
		} else {
			c = sizeof(int) - ((uintptr_t) s & (sizeof(int) - 1));
		}

		/* subtract leading bytes to be copied */
		n -= c;

		while (c) {

			d++;
			s++;

			(*d) = (*s);

			c--;
		}
	}

	/* if any bytes remain, copy the central section in more efficient
	 * word-size load/stores
	 * arch-specific implementations could use double-word stores for
	 * greater efficiency if available
	 */

	c = n / sizeof(int);

	while (c) {

		(*((int *) d)) = (*((int *) s));

		s += sizeof(int);
		d += sizeof(int);

		c--;
	}

	/* copy remainder */
	c = n & (sizeof(int) - 1);

	while (c) {

		d++;
		s++;

		(*d) = (*s);

		c--;
	}
}

static void memmove_bwd(char *d, const char *s, size_t n)
{
	size_t c;


	/* we copy back to front, so adjust pointers to the end
	 * of the segment
	 */
	s += n;
	d += n;

	c = (uintptr_t) s;

	if (((uintptr_t) s | (uintptr_t) d) & (sizeof(int) - 1)) {

		/* same as in memmove_fwd, but start at the end */

		if (n <= sizeof(int)) {
			c = n;
		} else if (((uintptr_t) s ^ (uintptr_t) d) & (sizeof(int) - 1)) {
			c = n;
		} else {
			c = ((uintptr_t) s) & (sizeof(int) - 1);
		}

		n -= c;

		while (c) {

			d--;
			s--;

			(*d) = (*s);

			c--;
		}
	}

	/* middle section */
	c = n / sizeof(int);

	while (c) {

		s -= sizeof(int);
		d -= sizeof(int);

		(*((int *) d)) = (*((int *) s));

		c--;
	}


	/* remainder */
	c = n & (sizeof(int) - 1);

	while (c) {

		d--;
		s--;

		(*d) = (*s);

		c--;
	}
}

#endif /* USE_OPTIMISATIONS */

