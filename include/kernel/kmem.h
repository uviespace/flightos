/**
 * @file include/kernel/kmem.h
 * @ingroup kmem
 */

#ifndef _KERNEL_KMEM_H_
#define _KERNEL_KMEM_H_

#include <stddef.h>

void *kmalloc(size_t size);
void *kzalloc(size_t size);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);

void kfree(void *ptr);

void *kmem_init(void);

#endif /* _KERNEL_KMEM_H_ */
