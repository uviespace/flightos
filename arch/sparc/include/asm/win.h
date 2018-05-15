/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SPARC_WIN_H
#define _SPARC_WIN_H


/* register window offsets */
#define RW_L0     0x00
#define RW_L1     0x04
#define RW_L2     0x08
#define RW_L3     0x0C
#define RW_L4     0x10
#define RW_L5     0x14
#define RW_L6     0x18
#define RW_L7     0x1C

#define RW_I0     0x20
#define RW_I1     0x24
#define RW_I2     0x28
#define RW_I3     0x2C
#define RW_I4     0x30
#define RW_I5     0x34
#define RW_I6     0x38
#define RW_I7     0x3C


#define LOAD_WINDOW(reg) \
	ldd	[%reg + RW_L0], %l0; \
	ldd	[%reg + RW_L2], %l2; \
	ldd	[%reg + RW_L4], %l4; \
	ldd	[%reg + RW_L6], %l6; \
	ldd	[%reg + RW_I0], %i0; \
	ldd	[%reg + RW_I2], %i2; \
	ldd	[%reg + RW_I4], %i4; \
	ldd	[%reg + RW_I6], %i6;

#define STORE_WINDOW(reg) \
	std	%l0, [%reg + RW_L0]; \
	std	%l2, [%reg + RW_L2]; \
	std	%l4, [%reg + RW_L4]; \
	std	%l6, [%reg + RW_L6]; \
	std	%i0, [%reg + RW_I0]; \
	std	%i2, [%reg + RW_I2]; \
	std	%i4, [%reg + RW_I4]; \
	std	%i6, [%reg + RW_I6];


#endif /* _SPARC_WIN_H */
