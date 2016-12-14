/**
 * @file include/kernel/err.h
 * @note from linux/include/linux/err.h
 */

#ifndef _KERNEL_ERR_H_
#define _KERNEL_ERR_H_

#include <compiler.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_ERRNO	4095

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)

static inline void *ERR_PTR(long error)
{
	return (void *) error;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

static inline bool IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

static inline bool IS_ERR_OR_NULL(const void *ptr)
{
	return unlikely(!ptr) || IS_ERR_VALUE((unsigned long)ptr);
}

#endif /* _KERNEL_ERR_H_ */
