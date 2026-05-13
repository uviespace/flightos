/*+
 * @file lib/memmove.c
 *
 * @ingroup string
 *
 * @brief implements memcpy() and memmove()
 *
 */


#include <kernel/kmem.h>
#include <kernel/export.h>
#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/printk.h>
#include <kernel/log2.h>
#include <kernel/bitops.h>
#include <kernel/kernel.h>
#include <kernel/tty.h>


union cpy64 {
	uint64_t v;
	struct {
		uint32_t h;
		uint32_t l;
	};
};


static inline void cpy_64_64(void **dst, const void **src, size_t *nn)
{
	size_t c;
	size_t n = (*nn);

	uint64_t *d;
	const uint64_t *s;

	register uint64_t r0, r1, r2, r3;


	d = (uint64_t *)(*dst);
	s = (uint64_t *)(*src);

	/* this may be lesrc efficient on platforms which do not
	 * have enough registers available, but 4 seems an ok choice
	 * (and it works perfectly fine on SPARC)
	 */
	c = n / sizeof(uint64_t) / 4;
	if (c < 4)
		goto tail;

	n -= c * sizeof(uint64_t) * 4;

	while (c--) {

		r0 = s[0];
		r1 = s[1];
		r2 = s[2];
		r3 = s[3];
		d[0] = r0;
		d[1] = r1;
		d[2] = r2;
		d[3] = r3;

		s += 4;
		d += 4;
	}

tail:
	/* copy the aligned tail end */
	c = n / sizeof(uint64_t);
	n -= c * sizeof(uint64_t);
	while (c--)
		(*((uint64_t *)d++)) = (*((uint64_t *)s++));

	(*nn)  = n;
	(*dst) = d;
	(*src) = s;
}


static inline void cpy_16_16(void **dst, const void **src, size_t *nn)
{
	size_t c;
	size_t n = (*nn);

	uint16_t *d;
	const uint16_t *s;

	register uint16_t r0, r1;


	d = (uint16_t *)(*dst);
	s = (uint16_t *)(*src);

	/* a loop of 2 half-words seems to be best (on SPARC) */
	c = n / sizeof(uint16_t) / 2;
	if (c < 2)
		goto exit;

	n -= c * sizeof(uint16_t) * 2;

	while (c--) {

		r0 = s[0];
		r1 = s[1];
		d[0] = r0;
		d[1] = r1;

		s += 2;
		d += 2;
	}
exit:
	(*nn)  = n;
	(*dst) = d;
	(*src) = s;
}


/* needed or sparc-gaisler-elf-gcc 13.2.1 (bcc-v2.3.1)
 * will not always generate 64-bit load instructions and thus ~40% slower code:
 *
 *	#pragma GCC optimize("no-strict-aliasing")
 *
 * this is default for the kernel build, so I'm not using it here
 */
static void cpy_64_32(void **dst, const void **src, size_t *nn)
{
	size_t c;
	size_t n = (*nn);

	uint32_t *d;
	const uint64_t *s;

	register union cpy64 r0;
	register union cpy64 r1;


	d = (uint32_t *)(*dst);
	s = (uint64_t *)(*src);

	c =  n / sizeof(uint64_t) / 2;
	n -= c * sizeof(uint64_t) * 2;

	if (c < 2)
		goto exit;

	while (c--) {

		r0.v = s[0];
		d[0] = r0.h;
		d[1] = r0.l;
		r1.v = s[1];
		d[2] = r1.h;
		d[3] = r1.l;

		s += 2;
		d += 4;
	}

exit:
	(*nn) = n;
	(*dst) = d;
	(*src) = s;
}

#if 0
/* dest is 64-bit aligned; doing this in partial 64-bit access compared
 * to a 32-bit only has no obvious drawback. However, when we can tell the
 * compiler how to use the target CPU's register layout, this has the
 * potential to become a lot faster, e.g  ~17% on SPARC
 */

static inline void cpy_32_64(void **dst, const void **src, size_t *nn)
{
	size_t c;
	size_t n = (*nn);

	const uint32_t *s;
	uint64_t *d;

	register union cpy64 r0 asm("%l0");
	register union cpy64 r1 asm("%l2");


	d = (uint64_t *)(*dst);
	s = (uint32_t *)(*src);

	c =  n / sizeof(uint64_t) / 2;
	n -= c * sizeof(uint64_t) * 2;

	while (c--) {


		r0.h = s[0];
		r0.l = s[1];
		d[0] = r0.v;
		r1.h = s[2];
		r1.l = s[3];
		d[1] = r1.v;

		s += 4;
		d += 2;
	}


	(*nn) = n;
	(*dst) = d;
	(*src) = s;
}
#endif

static void precondition_alignment(void **dst, const void **src, size_t *nn)
{
	size_t n = (*nn);

	uint32_t *d = (*dst);
	const uint32_t *s = (*src);

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
				s += 1;
				d += 1;
				n -= 1;
			}
		}
	}

	(*nn)  = n;
	(*dst) = d;
	(*src) = s;
}

static void memmove_fwd(void *d, const void *s, size_t n)
{
	size_t c;


	if (n < sizeof(uint32_t))
			goto tail_bytes;

	if (!IS_ALIGNED((uintptr_t)s | (uintptr_t)d, sizeof(uint64_t))) {

		if (IS_ALIGNED((uintptr_t)s | (uintptr_t)d, sizeof(uint32_t))) {

			if (IS_ALIGNED((uintptr_t)s, sizeof(uint64_t))) {
				/* src is 64-bit aligned */
				cpy_64_32(&d, &s, &n);
			} else {
				/* none are is 64-bit aligned but they are
				 * 32-bit aligned, so move up the to the
				 * next boundary
				 */

				(*((uint32_t *)d)) = (*((uint32_t *)s));
				s = (void *) ((uintptr_t)s + sizeof(uint32_t));
				d = (void *) ((uintptr_t)d + sizeof(uint32_t));
				n -= sizeof(uint32_t);

				goto double_copy;
			}

			goto tail_bytes;
		}

		/* we're off by 2 bytes, copy in 16 bit operations;
		 * there's no point in doing 32<->16 or 64<->16 implementations
		 * unless we really run on an architecture which supports
		 * extraction of half-words from a word-size register
		 */
		if (IS_ALIGNED((uintptr_t)s | (uintptr_t)d, sizeof(uint16_t))) {
				cpy_16_16(&d, &s, &n);
				goto tail_bytes;
		}

		/* here we are in the situation where the addresses are not
		 * aligned to word boundaries but can be aligned by advancing
		 * a few bytes. So copy these bytes, then move on to the more
		 * efficient double-word-sized copy below.
		 * If both addresses are always unaligned (i.e. off by 1 byte),
		 * just do a byte-wise copy as a last resort.
		 */

		if (n < sizeof(uint64_t)) {
			c = n;	/* there isn't much to copy ... */
		} else if (((uintptr_t) s ^ (uintptr_t) d) & (sizeof(uint64_t) - 1)) {
			goto tail_bytes; /* one pointer is always unaligned */
		} else {
			/* move to to 64-bit boundary */
			c = sizeof(uint64_t) - ((uintptr_t) s & (sizeof(uint64_t) - 1));
		}

		/* copy the head bytes until aligned */
		n -= c;
		while (c--)
			(*((*(uint8_t **)&d)++)) = (*((*(uint8_t **)&s)++));
	}

double_copy:
	cpy_64_64(&d, &s, &n);

tail_bytes:
	if (!n)
		return;

	while (n--)
		(*((*(uint8_t **)&d)++)) = (*((*(uint8_t **)&s)++));	/* fuck your lvalue */
}



static void memmove_bwd(char *d, const char *s, size_t n)
{
	size_t c;


	/* we copy back to front, so adjust pointers to the end
	 * of the segment
	 */
	s += n;
	d += n;

	if (((uintptr_t) s | (uintptr_t) d) & (sizeof(uint64_t) - 1)) {

		/* same as in memmove_fwd, but start at the end */

		if (n <= sizeof(uint64_t)) {
			c = n;
		} else if (((uintptr_t) s ^ (uintptr_t) d) & (sizeof(uint64_t) - 1)) {
			c = n;
		} else {
			c = ((uintptr_t) s) & (sizeof(uint64_t) - 1);
		}

		n -= c;

		while (c--) {

			d--;
			s--;

			(*d) = (*s);
		}
	}

	/* middle section */
	c = n / sizeof(uint64_t);

	while (c--) {

		s -= sizeof(uint64_t);
		d -= sizeof(uint64_t);

		(*((uint64_t *) d)) = (*((uint64_t *) s));
	}


	/* remainder */
	c = n & (sizeof(uint64_t) - 1);

	while (c--) {

		d--;
		s--;

		(*d) = (*s);
	}
}


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

	if ((uintptr_t)dest + n < (uintptr_t)src ||  (uintptr_t)src + n < (uintptr_t)dest)
		memmove_fwd(dest, src, n);
	else
		memmove_bwd(dest, src, n);

	return dest;
}
EXPORT_SYMBOL(memmove);

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
	memmove_fwd(dest, src, n);
	return dest;
}
EXPORT_SYMBOL(memcpy);
