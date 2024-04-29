/**
 * @file include/kernel/bitmap.h
 *
 * from linux/bitmap.h
 */


#ifndef _KERNEL_BITMAP_H_
#define _KERNEL_BITMAP_H_

#include <kernel/bitops.h>

#include <string.h>


void bitmap_print(const unsigned long *bitarr, unsigned long nr);


#define small_const_nbits(nbits) \
        (__builtin_constant_p(nbits) && (nbits) <= BITS_PER_LONG)

static inline void bitmap_zero(unsigned long *dst, unsigned int nbits)
{
        if (small_const_nbits(nbits))
                *dst = 0UL;
        else {
                unsigned int len = BITS_TO_LONGS(nbits) * sizeof(unsigned long);
                memset(dst, 0, len);
        }
}


#endif /*_KERNEL_BITMAP_H_*/
