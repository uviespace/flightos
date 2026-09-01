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

/**
 * @brief encode an error code into a pointer value
 * @param error: error code (negative)
 * @return pointer representation of the error
 */
static inline void *ERR_PTR(long error)
{
	return (void *) error;
}

/**
 * @brief decode an error pointer into an error code
 * @param ptr: error pointer returned by ERR_PTR
 * @return the encoded error code
 */
static inline long PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

/**
 * @brief check whether a pointer is an encoded error
 * @param ptr: pointer to test
 * @return true if ptr is an error pointer, false otherwise
 */
static inline bool IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

/**
 * @brief check whether a pointer is NULL or an encoded error
 * @param ptr: pointer to test
 * @return true if ptr is NULL or an error pointer, false otherwise
 */
static inline bool IS_ERR_OR_NULL(const void *ptr)
{
	return unlikely(!ptr) || IS_ERR_VALUE((unsigned long)ptr);
}

#endif /* _KERNEL_ERR_H_ */
