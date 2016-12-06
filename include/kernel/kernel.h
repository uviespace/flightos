#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <compiler.h>


#define ALIGN_MASK(x, mask)    (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)            ALIGN_MASK(x, (typeof(x))(a) - 1)


/* this is a bit crude, but must do for now */
#define panic(x) {}while(1)

#include <stdio.h>
#define BUG() do { \
        printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
        panic("BUG!"); \
} while (0)

#define BUG_ON(condition) do { if (unlikely(condition)) BUG(); } while (0)


#endif /* _KERNEL_H_ */
