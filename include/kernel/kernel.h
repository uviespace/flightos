#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <compiler.h>

#define ALIGN_MASK(x, mask)    (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)            ALIGN_MASK(x, (typeof(x))(a) - 1)
#define ALIGN_PTR(x, a)        (typeof(x)) ALIGN((unsigned long) x, a)


/* this is a bit crude, but must do for now */
#define panic(x) {} while(1);

/* the BUG() macros may be repurposed to log an error and boot to safe mode! */
#include <stdio.h>
#define BUG() do { \
        printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
        panic("BUG!"); \
} while (0)

#define BUG_ON(condition) do { if (unlikely(condition)) BUG(); } while (0)


#define offset_of(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

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



#endif /* _KERNEL_H_ */
