/**
 * @file    sparc/include/asm/io.h
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

#ifndef _ARCH_SPARC_ASM_IO_H_
#define _ARCH_SPARC_ASM_IO_H_

#include <stdint.h>
#include <uapi/asi.h>

/*
 * convention/calls same as in linux kernel (see arch/sparc/include/asm/io_32.h)
 */



#define __raw_readb __raw_readb
static inline uint8_t __raw_readb(const volatile void *addr)
{
	uint8_t ret;

	__asm__ __volatile__("lduba	[%1] %2, %0\n\t"
			     : "=r" (ret)
			     : "r" (addr), "i" (ASI_LEON_NOCACHE));

	return ret;
}

#define __raw_readw __raw_readw
static inline uint16_t __raw_readw(const volatile void *addr)
{
	uint16_t ret;

	__asm__ __volatile__("lduha	[%1] %2, %0\n\t"
			     : "=r" (ret)
			     : "r" (addr), "i" (ASI_LEON_NOCACHE));

	return ret;
}


#define __raw_readl __raw_readl
static inline uint32_t __raw_readl(const volatile void *addr)
{
	uint32_t ret;

	__asm__ __volatile__("lda	[%1] %2, %0\n\t"
			     : "=r" (ret)
			     : "r" (addr), "i" (ASI_LEON_NOCACHE));

	return ret;
}

#define __raw_writeb __raw_writeb
static inline void __raw_writeb(uint8_t w, const volatile void *addr)
{
	__asm__ __volatile__("stba	%r0, [%1] %2\n\t"
			     :
			     : "Jr" (w), "r" (addr), "i" (ASI_LEON_NOCACHE));
}

#define __raw_writew __raw_writew
static inline void __raw_writew(uint16_t w, const volatile void *addr)
{
	__asm__ __volatile__("stha	%r0, [%1] %2\n\t"
			     :
			     : "Jr" (w), "r" (addr), "i" (ASI_LEON_NOCACHE));
}


#define __raw_writel __raw_writel
static inline void __raw_writel(uint32_t l, const volatile void *addr)
{
	__asm__ __volatile__("sta	%r0, [%1] %2\n\t"
			     :
			     : "Jr" (l), "r" (addr), "i" (ASI_LEON_NOCACHE));
}

#ifndef ioread8
#define ioread8(X)                      __raw_read8(X)
#endif

#ifndef iowrite8
#define iowrite8(X)                     __raw_write8(X)
#endif

#ifndef ioread16be
#define ioread16be(X)                   __raw_readw(X)
#endif

#ifndef ioread32be
#define ioread32be(X)                   __raw_readl(X)
#endif

#ifndef iowrite16be
#define iowrite16be(val,X)              __raw_writew(val,X)
#endif

#ifndef iowrite32be
#define iowrite32be(val,X)              __raw_writel(val,X)
#endif

#endif /* _ARCH_SPARC_ASM_IO_H_ */
