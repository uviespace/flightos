/**
 * @file include/kernel/printk.h
 *
 * @brief kernel print function declaration
 *
 * Declares printk(), the kernel's printf-like logging function, gated on the
 * CONFIG_KERNEL_PRINTK configuration option.
 */

#ifndef _KERNEL_PRINTK_H_
#define _KERNEL_PRINTK_H_

#include <kernel/kernel_levels.h>

#ifdef CONFIG_KERNEL_PRINTK
/**
 * @brief kernel print function with formatted output
 * @param fmt: printf-style format string
 * @return number of characters printed, or negative error code
 */
int printk(const char *fmt, ...);
#else
#define printk(fmt, ...)
#endif

#endif /* _KERNEL_PRINTK_H_ */
