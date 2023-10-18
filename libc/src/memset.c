#include <stddef.h>
#include <stdint.h>

#ifndef USE_OPTIMISATIONS

/**
 * @brief fills a memory area with the constant byte c
 *
 * @param s a pointer to the memory area
 * @param c the byte to set
 * @param n the number of bytes to fill
 *
 * @note this function provides some optimisation by using word-size stores
 *	 when possible
 *
 * @returns a pointer to the memory area s
 */

void *memset(void *s, int c, size_t n)
{
	volatile char *p = s;

	while (n--)
		*p++ = c;

	return s;
}

#else /* USE_OPTIMISATIONS */

/**
 * @brief fills a memory area with the constant byte c
 *
 * @param s a pointer to the memory area
 * @param c the byte to set
 * @param n the number of bytes to fill
 *
 * @note this function provides some optimisation by using word-size stores
 *	 when possible
 *
 * @returns a pointer to the memory area s
 */

#define IS_ALIGNED(x, a)       (((x) & ((typeof(x))(a) - 1)) == 0)
#if 0
void *memset(void *s, int c, size_t n)
{
	size_t i;

	char byte;
	char *p;

	/* memset accepts only the least significant BYTE to set,
	 * so we first prep by filling our integer to full width
	 * with that byte
	 */
	p    = (char *) &c;
	byte = (char) (c & 0xFF);
	for (i = 0; i < sizeof(int); i++)
		p[i] = byte;


	/* now start filling the memory area */
	p = s;


	while (n) {
		/* fill the head bytes until aligned */
		if (n > sizeof(int)) {
			if (IS_ALIGNED((uintptr_t) p, sizeof(int))) {
				break;
			}
		}

		*p++ = byte;
		n--;
	}

	/* do larger stores in the central segment
	 * for arch specific implementations, you could do larger than
	 * int stores with precomputed offsets for increased efficiency
	 */

	i = n / sizeof(int);

	while (i) {

		(*(int *) p) = c;

		p += sizeof(int);
		n -= sizeof(int);
	}

	/* fill remaining tail bytes */

	n = n & (sizeof(int) - 1);

	while (n) {
		*p++ = byte;
		n--;
	}

	return s;
}
#else
void *memset(void *s, int c, size_t n)
{
	size_t i;
	char byte;
	char *p;

	/* memset accepts only the least significant BYTE to set,
	 * so we first prep by filling our integer to full width
	 * with that byte
	 */
	p    = (char *) &c;
	byte = (char) (c & 0xFF);
	for (i = 0; i < sizeof(int); i++)
		p[i] = byte;


	/* now start filling the memory area */
	p = s;

	while (n) {
		/* do larger stores in the central segment
		 * for arch specific implementations, you could do larger than
		 * int stores with precomputed offsets for increased efficiency
		 */
		if (n > sizeof(int)) {
			if (IS_ALIGNED((uintptr_t) p, sizeof(int))) {
				(* (int *) p) = c;
				p += 4;
				n -= 4;
				continue;
			}
		}

		*p++ = (char) (c & 0xFF);
		n--;
	}

	return s;
}
#endif

#endif /* USE_OPTIMISATIONS */
