/**
 * @file include/kernel/kmem.h
 * @ingroup kmem
 */

#ifndef _KERNEL_KMEM_H_
#define _KERNEL_KMEM_H_

#include <stddef.h>

/**
 * @brief allocate memory from the kernel heap
 * @param size: number of bytes to allocate
 * @return pointer to allocated memory, or NULL on failure
 */
void *kmalloc(size_t size);

/**
 * @brief allocate zero-initialized memory from the kernel heap
 * @param size: number of bytes to allocate
 * @return pointer to allocated zero-filled memory, or NULL on failure
 */
void *kzalloc(size_t size);

/**
 * @brief allocate an array of zero-initialized memory elements
 * @param nmemb: number of elements
 * @param size: size of each element in bytes
 * @return pointer to allocated zero-filled memory, or NULL on failure
 */
void *kcalloc(size_t nmemb, size_t size);

/**
 * @brief reallocate memory from the kernel heap
 * @param ptr: pointer to previously allocated memory (may be NULL)
 * @param size: new size in bytes
 * @return pointer to reallocated memory, or NULL on failure
 */
void *krealloc(void *ptr, size_t size);

/**
 * @brief allocate page-aligned memory from the kernel heap
 * @param size: number of bytes to allocate (rounded up to page alignment)
 * @return pointer to page-aligned allocated memory, or NULL on failure
 */
void *kpalloc(size_t size);

/**
 * @brief allocate page-aligned zero-initialized memory array
 * @param nmemb: number of elements
 * @param size: size of each element in bytes
 * @return pointer to page-aligned zero-filled memory, or NULL on failure
 */
void *kpcalloc(size_t nmemb, size_t size);

/**
 * @brief allocate page-aligned zero-initialized memory
 * @param size: number of bytes to allocate
 * @return pointer to page-aligned zero-filled memory, or NULL on failure
 */
void *kpzalloc(size_t size);


/**
 * @brief free kernel memory
 * @param ptr: pointer to memory previously allocated by kmalloc/kzalloc/etc.
 */
void kfree(void *ptr);


/**
 * @brief initialize the kernel memory allocator
 * @return pointer to the initialized kernel heap base
 */
void *kmem_init(void);

#endif /* _KERNEL_KMEM_H_ */
