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
 * Stolen from Linux (hpi_internal.h, compiler.h)
 */
#define compile_time_assert(cond, msg) typedef char ASSERT_##msg[(cond) ? 1 : -1]

#define __caller(x) __builtin_return_address((x))

#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

#define BUILD_BUG_ON_ZERO(e) (0)

/**
 * same with the stuff below
 */

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)


#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

/* optimisation barrier */
#define barrier() __asm__ __volatile__("": : :"memory")


/** GCC specific stuff */
#ifdef __GNUC__
#define GCC_VERSION (__GNUC__ * 10000 \
		     + __GNUC_MINOR__ * 100 \
		     + __GNUC_PATCHLEVEL__)
#endif


#ifdef __GNUC__
#define __diag_GCC(version, severity, s) \
	__diag_GCC_ ## version(__diag_GCC_ ## severity s)
#endif

#define __diag_GCC_ignore	ignored
#define __diag_GCC_warn		warning
#define __diag_GCC_error	error

#ifdef __GNUC__
#define __diag_str1(s)		#s
#define __diag_str(s)		__diag_str1(s)
#define __diag(s)		_Pragma(__diag_str(GCC diagnostic s))
#endif

#if GCC_VERSION >= 70000
#define __diag_GCC_7(s)		__diag(s)
#else
#define __diag_GCC_7(s)
#endif

#ifndef __diag
#define __diag(s)
#endif

#ifndef __diag_GCC
#define __diag_GCC(version, severity, string)
#endif

#define __diag_pop()	__diag(pop)
#define __diag_push()	__diag(push)
#define __diag_ignore(compiler, version, option, comment) \
	__diag_ ## compiler(version, ignore, option)





#endif /* _KERNEL_COMPILER_H_ */
