/**
 * @file include/kernel/sbrk.h
 */

#ifndef _KERNEL_SBRK_H_
#define _KERNEL_SBRK_H_

#include <kernel/types.h>

void *kernel_sbrk(intptr_t increment);

#endif /* _KERNEL_SBRK_H_ */
