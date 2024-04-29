/**
 * @file include/kernel/printk.h
 */

#ifndef _KERNEL_KERNEL_LEVELS_H_
#define _KERNEL_KERNEL_LEVELS_H_

#include <kernel/kernel_levels.h>

#ifdef CONFIG_KERNEL_PRINTK
int printk(const char *fmt, ...);
#else
#define printk(fmt, ...)
#endif

#endif /* _KERNEL_KERNEL_LEVELS_H_ */
