#include <compiler.h>
#include <stdint.h>

__diag_push()
__diag_ignore(GCC, 7, "-Wlong-long", "we need this for 64 bit types")

//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
double __floatundidf(uint64_t a) {
	static const double twop52 = 4503599627370496.0;           // 0x1.0p52
	static const double twop84 = 19342813113834066795298816.0; // 0x1.0p84
	static const double twop84_plus_twop52 = 19342813118337666422669312.0; // 0x1.00000001p84
	union {
		uint64_t x;
		double d;
	} high = {.d = twop84};
	union {
		uint64_t x;
		double d;
	} low = {.d = twop52};
	high.x |= a >> 32;
	low.x |= a & 0x00000000ffffffffULL;
	const double result = (high.d - twop84_plus_twop52) + low.d;
	return result;
}

__diag_pop()	/* -Wlong-long */
