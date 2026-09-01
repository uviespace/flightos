/**
 * @file include/kernel/sbrk.h
 * @ingroup kmem
 *
 * @brief Abstract program-break interface used by the kernel heap.
 *
 * The implementation controls the virtual heap range and, where applicable,
 * its page mappings. The implementation is architecture-specific.
 */

#ifndef _KERNEL_SBRK_H_
#define _KERNEL_SBRK_H_

#include <kernel/types.h>

/**
 * @brief change the size of the program break for dynamic memory allocation
 * @param increment: number of bytes to expand (positive) or contract (negative) the heap
 * @return previous program break address, or (void *)-1 on failure
 */
void *kernel_sbrk(intptr_t increment);

#endif /* _KERNEL_SBRK_H_ */
