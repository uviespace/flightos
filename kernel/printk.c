/**
 * @file kernel/printk.c
 *
 * @note printk levels obviously stolen from linux (include/linux/printk.h)
 * @copyright Linus Torvalds et. al.
 *
 * TODO this obviously needs custom support when not using newlib/bcc
 */


#include <stdarg.h>
#include <stdio.h>

#include <kernel/kernel_levels.h>

#if defined(CONFIG_KERNEL_LEVEL)
#define KERNEL_LEVEL (CONFIG_KERNEL_LEVEL + '0')
#else
#define KERNEL_LEVEL '7'
#endif

static int printk_get_level(const char *buffer)
{
	if (buffer[0] == KERN_SOH_ASCII && buffer[1]) {
		switch (buffer[1]) {
		case '0' ... '7':
		case 'd':       /* KERN_DEFAULT */
			return buffer[1];
		}
	}
	return 0;
}


static inline const char *printk_skip_level(const char *buffer)
{
	if (printk_get_level(buffer))
		return buffer + 2;

	return buffer;
}

/**
 * @brief see printf(3)
 *
 */

int printk(const char *fmt, ...)
{
	int ret = 0;
	int level;

	va_list args;

	
	level = printk_get_level(fmt); 

	va_start(args, fmt);

	if (level) {
		if (level < KERNEL_LEVEL) 
			ret = vprintf(printk_skip_level(fmt), args);
	} else {
		ret = vprintf(fmt, args);
	}

	va_end(args);

	return ret;
}
