/**
 * @file   include/kernel/compiler.h
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   2015
 *
 * @copyright GPLv2
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @brief a collection of preprocessor macros
 */


#ifndef _KERNEL_COMPILER_H_
#define _KERNEL_COMPILER_H_


/**
 * Compile time check usable outside of function scope.
 * Stolen from Linux (hpi_internal.h)
 */
#define compile_time_assert(cond, msg) typedef char ASSERT_##msg[(cond) ? 1 : -1]

#define __caller(x) __builtin_return_address((x))

/**
 * same with the stuff below
 */

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)


#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

/* optimisation barrier */
#define barrier() __asm__ __volatile__("": : :"memory")

#endif /* _KERNEL_COMPILER_H_ */
