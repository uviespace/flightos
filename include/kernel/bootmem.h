/**
 * @file include/kernel/bootmem.h
 * @ingroup kmem
 *
 * @brief High-level interface to early physical-memory allocation.
 *
 * This interface is used while the architecture brings up its physical memory
 * pools. The backing allocator and the lifetime rules are architecture-specific.
 * See the architecture group for the implementation details.
 */

#ifndef _KERNEL_BOOTMEM_H_
#define _KERNEL_BOOTMEM_H_

#include <kernel/types.h>

/**
 * @brief initialize the early boot memory allocator
 */
void bootmem_init(void);

/**
 * @brief allocate memory from the boot memory allocator
 * @param size: number of bytes to allocate
 * @return pointer to allocated memory, or NULL on failure
 */
void *bootmem_alloc(size_t size);

/**
 * @brief free memory back to the boot memory allocator
 * @param ptr: pointer to memory previously allocated by bootmem_alloc
 */
void bootmem_free(void *ptr);

#endif /* _KERNEL_BOOTMEM_H_ */
