/**
 * @file include/kernel/types.h
 */

#ifndef _KERNEL_TYPES_H_
#define _KERNEL_TYPES_H_

#include <stddef.h>

#include <stdbool.h>

#ifdef __GNUC__
#define GCC_VERSION (__GNUC__ * 10000 \
		     + __GNUC_MINOR__ * 100 \
		     + __GNUC_PATCHLEVEL__)
#endif


/* BCC is at least 4.4.2 */
#if defined(GCC_VERSION) && (GCC_VERSION >= 404020)
#include <stdint.h>
#else
/* this must do... */
typedef unsigned long uintptr_t;
typedef   signed long  intptr_t;

typedef unsigned long long uint64_t;
typedef   signed long long  int64_t;

typedef unsigned long  uint32_t;
typedef   signed long   int32_t;

typedef unsigned short uint16_t;
typedef   signed short  int16_t;

typedef unsigned char uint8_t;
typedef   signed char  int8_t;

#endif

#ifndef __SIZEOF_LONG__
#define __SIZEOF_LONG__ (sizeof(long))
#endif



#endif /* _KERNEL_TYPES_H_ */
