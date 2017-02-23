/**
 * @file include/kernel/bootmem.h
 */

#ifndef _KERNEL_BOOTMEM_H_
#define _KERNEL_BOOTMEM_H_

#include <kernel/types.h>

void bootmem_init(void);
void *bootmem_alloc(size_t size);
void bootmem_free(void *ptr);

#endif /* _KERNEL_BOOTMEM_H_ */
