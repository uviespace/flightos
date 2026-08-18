#include <compiler.h>
#include <stdint.h>

__diag_push()
__diag_ignore(GCC, 7, "-Wlong-long", "we need this for 64 bit types")

/*
 Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 See https://llvm.org/LICENSE.txt for license information.
 SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

===----------------------------------------------------------------------===*/

uint64_t __fixunsdfdi(double a) {
	uint32_t high = a / 4294967296.f;               /* a / 0x1p32f; */
	uint32_t low  = a - (double)high * 4294967296.f; /* high * 0x1p32f; */
	if (a <= 0.0)
		return 0;
	return ((uint64_t)high << 32) | low;
}

__diag_pop()	/* -Wlong-long */
