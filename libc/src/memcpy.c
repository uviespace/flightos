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

void *memmove(void *dest, const void *src, size_t n);

#define IS_ALIGNED(x, a) (((x) & ((typeof(x))(a) - 1)) == 0)

static void precondition_alignment(void **dd, const void **ss, size_t *nn)
{
	size_t n = (*nn);

	char *d = (*dd);
	const char *s = (*ss);

	/* fuck off, gcc
	 *
	 * Since a cpy_64_32() will be faster (on a SPARC at least) than
	 * a cpy_32_64, we can move forward by a word so we always end
	 * up with the source pointer being aligned to 8 and use the
	 * (+50%) faster option.
	 * For some reason, gcc refuses to optimise the code path with double
	 * loads if it suspects that the source pointer have non-8 alignment
	 * and no __builtin_assume_aligned() or anything of the sort will
	 * have an influence on that decision. The check and pointer forwards
	 * are identical to what I was doing in memmove_fwd(). As I mostly care
	 * about memcpy performance, this solution is fine for now.
	 */

	if (n < 4)
		return;

	if (!IS_ALIGNED((uintptr_t)s | (uintptr_t)d, sizeof(uint64_t))) {
		if (IS_ALIGNED((uintptr_t)s | (uintptr_t)d, sizeof(uint32_t))) {
			if (IS_ALIGNED((uintptr_t)d, sizeof(uint64_t))) {
				(*((uint32_t *)d)) = (*((uint32_t *)s));
				s += sizeof(uint32_t);
				d += sizeof(uint32_t);
				n -= sizeof(uint32_t);
			}
		}
	}

	(*nn) = n;
	(*dd) = d;
	(*ss) = s;

}

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
	precondition_alignment(&dest, &src, &n);
	return memmove(dest, src, n);
}

#endif /* USE_OPTIMISATIONS */
