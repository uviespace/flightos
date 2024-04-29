/**
 * @file include/kernel/types.h
 */

#ifndef _KERNEL_TYPES_H_
#define _KERNEL_TYPES_H_

#include <compiler.h>

#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>


/* d'oh! -.- */
#if defined(CONFIG_LEON2) || defined(CONFIG_LEON3)
#define __BIG_ENDIAN_BITFIELD __BIG_ENDIAN_BITFIELD
#endif


/* BCC is at least 4.4.2 */
#if defined(GCC_VERSION) && (GCC_VERSION >= 40402)
#include <stdint.h>
#else
#include <stdint.h>
#if 0 /* XXX need CLANG FIX */
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

compile_time_assert(sizeof(uint64_t) == 8, TYPES_UINT64_SIZE_INVALID);
compile_time_assert(sizeof( int64_t) == 8, TYPES__INT64_SIZE_INVALID);
compile_time_assert(sizeof(uint32_t) == 4, TYPES_UINT32_SIZE_INVALID);
compile_time_assert(sizeof( int32_t) == 4, TYPES__INT32_SIZE_INVALID);
compile_time_assert(sizeof(uint16_t) == 2, TYPES_UINT16_SIZE_INVALID);
compile_time_assert(sizeof( int16_t) == 2, TYPES__INT16_SIZE_INVALID);
compile_time_assert(sizeof(uint8_t)  == 1, TYPES_UINT_8_SIZE_INVALID);
compile_time_assert(sizeof( int8_t)  == 1, TYPES__INT_8_SIZE_INVALID);
#endif
#endif

#ifndef __SIZEOF_LONG__
#define __SIZEOF_LONG__ (sizeof(long))
#endif






#endif /* _KERNEL_TYPES_H_ */
