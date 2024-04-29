#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <compiler.h>
#include <kernel/printk.h>
#include <kernel/reboot.h>

#define ALIGN_MASK(x, mask)    (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)            ALIGN_MASK(x, (typeof(x))(a) - 1)
#define ALIGN_PTR(x, a)        (typeof(x)) ALIGN((unsigned long) x, a)
#define IS_ALIGNED(x, a)       (((x) & ((typeof(x))(a) - 1)) == 0)


#define panic(x) do {machine_halt();} while(1);

/* the BUG() macros may be repurposed to log an error and boot to safe mode! */
#define BUG() do { \
        pr_emerg("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
        panic(); \
} while (0)

#define BUG_ON(condition) do { if (unlikely(condition)) BUG(); } while (0)

#define MARK() do { \
        printk("MARK: %s:%d\n", __func__, __LINE__); \
} while (0)



#define offset_of(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define member_size(TYPE, MEMBER) (sizeof(((TYPE *)0)->MEMBER))

/* linux/kernel.h */
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offset_of(type,member) );})

/* Indirect stringification.  Doing two levels allows the parameter to be a
 * macro itself.  For example, compile with -DFOO=bar, __stringify(FOO)
 * converts to "bar".
 */
__extension__
#define __stringify_1(x...)     #x
#define __stringify(x...)       __stringify_1(x)



/*
 * Basic Moron Protector (BMP)™
 */
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

#define min(x, y) ({				\
	typeof(x) _min1 = (x);			\
	typeof(y) _min2 = (y);			\
	(void) (&_min1 == &_min2);		\
	_min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })


#define clamp(val, min, max) ({			\
	typeof(val) __val = (val);		\
	typeof(min) __min = (min);		\
	typeof(max) __max = (max);		\
	(void) (&__val == &__min);		\
	(void) (&__val == &__max);		\
	__val = __val < __min ? __min: __val;	\
	__val > __max ? __max: __val; })



#endif /* _KERNEL_H_ */
