#include <stdint.h>
#include <stddef.h>


#ifndef USE_OPTIMISATIONS

/**
 * @brief copy a memory area
 *
 * @param dest the destination memory area
 * @param src the source memory area
 * @param n the number of bytes to copy
 *
 * @returns a pointer to dest
 */

void *memcpy(void *dest, const void *src, size_t n)
{
	char *d;

	const char *s;


	d = dest;
	s = src;

	while (n--)
		*(d++) = *(s++);

	return d;
}

#else /* USE_OPTIMISATIONS */
/* now in memmove.c */
#endif /* USE_OPTIMISATIONS */
