/**
 * @file    io.h
 *
 * @copyright GPLv2
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @brief a collection of accessor functions and macros to perform unbuffered
 *        access to memory or hardware registers
 */

#ifndef _IO_H_
#define _IO_H_

#include <stdint.h>


/*
 * raw "register" io
 * convention/calls same as in linux kernel (see arch/sparc/include/asm/io_32.h)
 * @note: SPARSE macros are removed, we probably won't use it for the foreseeable
 *        future
 * @note need only big endian functions for now
 */


#if (__sparc__)

#define ASI_LEON_NOCACHE        0x01	/* force cache miss */

static inline uint8_t __raw_readb(const volatile void *addr)
{
	uint8_t ret;

	__asm__ __volatile__("lduba	[%1] %2, %0\n\t"
			     : "=r" (ret)
			     : "r" (addr), "i" (ASI_LEON_NOCACHE));

	return ret;
}

static inline uint16_t __raw_readw(const volatile void *addr)
{
	uint16_t ret;

	__asm__ __volatile__("lduha	[%1] %2, %0\n\t"
			     : "=r" (ret)
			     : "r" (addr), "i" (ASI_LEON_NOCACHE));

	return ret;
}


static inline uint32_t __raw_readl(const volatile void *addr)
{
	uint32_t ret;

	__asm__ __volatile__("lda	[%1] %2, %0\n\t"
			     : "=r" (ret)
			     : "r" (addr), "i" (ASI_LEON_NOCACHE));

	return ret;
}

static inline void __raw_writeb(uint8_t w, const volatile void *addr)
{
	__asm__ __volatile__("stba	%r0, [%1] %2\n\t"
			     :
			     : "Jr" (w), "r" (addr), "i" (ASI_LEON_NOCACHE));
}

static inline void __raw_writew(uint16_t w, const volatile void *addr)
{
	__asm__ __volatile__("stha	%r0, [%1] %2\n\t"
			     :
			     : "Jr" (w), "r" (addr), "i" (ASI_LEON_NOCACHE));
}


static inline void __raw_writel(uint32_t l, const volatile void *addr)
{
	__asm__ __volatile__("sta	%r0, [%1] %2\n\t"
			     :
			     : "Jr" (l), "r" (addr), "i" (ASI_LEON_NOCACHE));
}


#else

static inline uint8_t __raw_readb(const volatile void *addr)
{
        return *(const volatile uint8_t *)addr;
}

static inline uint16_t __raw_readw(const volatile void *addr)
{
        return *(const volatile uint16_t *)addr;
}

static inline uint32_t __raw_readl(const volatile void *addr)
{
        return *(const volatile uint32_t *)addr;
}

static inline void __raw_writeb(uint8_t w, volatile void *addr)
{
        *(volatile uint8_t *)addr = w;
}

static inline void __raw_writew(uint16_t w, volatile void *addr)
{
        *(volatile uint16_t *)addr = w;
}

static inline void __raw_writel(uint32_t l, volatile void *addr)
{
        *(volatile uint32_t *)addr = l;
}

#endif


#define ioread8(X)                      __raw_read8(X)
#define iowrite8(X)                     __raw_write8(X)

#define ioread16be(X)                   __raw_readw(X)
#define ioread32be(X)                   __raw_readl(X)

#define iowrite16be(val,X)              __raw_writew(val,X)
#define iowrite32be(val,X)              __raw_writel(val,X)



#endif
