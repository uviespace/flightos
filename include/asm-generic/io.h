/**
 * @file    include/asm-generic/io.h
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
 *        access to memory or hardware registers (generic variant)
 *
 * @note same conventions as linux kernel (see include/asm-generic/io.h)
 *
 * @todo since we really need only big endian functions for now, we don't do
 *	 byte swaps (also we don't really need the functions in this file at
 *	 this time)
 */

#ifndef _ASM_GENERIC_IO_H_
#define _ASM_GENERIC_IO_H_

#include <stdint.h>


#ifndef __raw_readb
#define __raw_readb __raw_readb
static inline uint8_t __raw_readb(const volatile void *addr)
{
        return *(const volatile uint8_t *)addr;
}
#endif

#ifndef __raw_readw
#define __raw_readw __raw_readw
static inline uint16_t __raw_readw(const volatile void *addr)
{
        return *(const volatile uint16_t *)addr;
}
#endif

#ifndef __raw_readl
#define __raw_readl __raw_readl
static inline uint32_t __raw_readl(const volatile void *addr)
{
        return *(const volatile uint32_t *)addr;
}
#endif

#ifndef __raw_writeb
#define __raw_writeb __raw_writeb
static inline void __raw_writeb(uint8_t w, volatile void *addr)
{
        *(volatile uint8_t *)addr = w;
}
#endif

#ifndef __raw_writew
#define __raw_writew __raw_writew
static inline void __raw_writew(uint16_t w, volatile void *addr)
{
        *(volatile uint16_t *)addr = w;
}
#endif

#ifndef __raw_writel
#define __raw_writel __raw_writel
static inline void __raw_writel(uint32_t l, volatile void *addr)
{
        *(volatile uint32_t *)addr = l;
}
#endif


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


#endif
