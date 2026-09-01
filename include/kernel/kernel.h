/**
 * @file include/kernel/kernel.h
 *
 * @brief core kernel utility macros
 *
 * Provides alignment, container-of, min/max and other low-level helpers used
 * throughout the kernel.
 */

#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <compiler.h>
#include <kernel/printk.h>
#include <kernel/reboot.h>

/**
 * @brief align a value up to an arbitrary mask
 * @param x: the value to align
 * @param mask: the alignment mask
 * @return the aligned value
 */
#define ALIGN_MASK(x, mask)    (((x) + (mask)) & ~(mask))

/**
 * @brief align a value up to a power-of-two alignment
 * @param x: the value to align
 * @param a: the alignment (a power of two)
 * @return the aligned value
 */
#define ALIGN(x, a)            ALIGN_MASK(x, (typeof(x))(a) - 1)

/**
 * @brief align a pointer up to a power-of-two alignment
 * @param x: the pointer to align
 * @param a: the alignment (a power of two)
 * @return the aligned pointer
 */
#define ALIGN_PTR(x, a)        (typeof(x)) ALIGN((unsigned long) x, a)

/**
 * @brief test whether a value is aligned to a power-of-two alignment
 * @param x: the value to test
 * @param a: the alignment (a power of two)
 * @return non-zero if aligned, 0 otherwise
 */
#define IS_ALIGNED(x, a)       (((x) & ((typeof(x))(a) - 1)) == 0)


/** @brief halt the machine and enter an infinite loop */
#define panic(x) do {machine_halt(x);} while(1);

/* the BUG() macros may be repurposed to log an error and boot to safe mode! */
/** @brief log a kernel bug and halt */
#define BUG() do { \
        pr_emerg("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
        panic(REBOOT_UNKNOWN); \
} while (0)

/** @brief trigger BUG() if the given condition is true */
#define BUG_ON(condition) do { if (unlikely(condition)) BUG(); } while (0)

/** @brief print a MARK: debug message with function and line */
#define MARK() do { \
        printk("MARK: %s:%d\n", __func__, __LINE__); \
} while (0)



/**
 * @brief get the byte offset of a member within a structure type
 * @param TYPE: the structure type
 * @param MEMBER: the member name
 * @return the offset of the member, in bytes
 */
#define offset_of(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 * @brief get the size of a member of a structure type
 * @param TYPE: the structure type
 * @param MEMBER: the member name
 * @return the size of the member, in bytes
 */
#define member_size(TYPE, MEMBER) (sizeof(((TYPE *)0)->MEMBER))

/* linux/kernel.h */
/**
 * @brief get a pointer to the containing structure from a member pointer
 * @param ptr: pointer to the member
 * @param type: the containing structure type
 * @param member: the member name
 * @return pointer to the containing structure
 */
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offset_of(type,member) );})

/* Indirect stringification.  Doing two levels allows the parameter to be a
 * macro itself.  For example, compile with -DFOO=bar, __stringify(FOO)
 * converts to "bar".
 */
__extension__
#define __stringify_1(x...)     #x

/** @brief stringify a macro argument (even if it is itself a macro) */
#define __stringify(x...)       __stringify_1(x)



/*
 * Basic Moron Protector (BMP)™
 */
/** @brief a one-shot guard that triggers BUG() on the second entry */
#define BMP() do {				\
	static enum  {DISABLED, ENABLED} bmp;	\
						\
	if (bmp)				\
		BUG();				\
						\
	bmp = ENABLED;			\
} while (0);


/* min()/max()/clamp() macros with strict type checking
 * (ripped off from linux/kernel.h)
 */

/**
 * @brief minimum of two values (with strict type checking)
 * @param x: first value
 * @param y: second value
 * @return the smaller of x and y
 */
#define min(x, y) ({				\
	typeof(x) _min1 = (x);			\
	typeof(y) _min2 = (y);			\
	(void) (&_min1 == &_min2);		\
	_min1 < _min2 ? _min1 : _min2; })

/**
 * @brief maximum of two values (with strict type checking)
 * @param x: first value
 * @param y: second value
 * @return the larger of x and y
 */
#define max(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })


/**
 * @brief clamp a value between a minimum and maximum
 * @param val: the value to clamp
 * @param min: the lower bound
 * @param max: the upper bound
 * @return val clamped into the range [min, max]
 */
#define clamp(val, min, max) ({			\
	typeof(val) __val = (val);		\
	typeof(min) __min = (min);		\
	typeof(max) __max = (max);		\
	(void) (&__val == &__min);		\
	(void) (&__val == &__max);		\
	__val = __val < __min ? __min: __val;	\
	__val > __max ? __max: __val; })



#endif /* _KERNEL_H_ */
