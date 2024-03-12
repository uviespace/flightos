/**
 * @file include/kernel/user.h
 */

#ifndef _KERNEL_USER_H_
#define _KERNEL_USER_H_

#ifdef CONFIG_ARCH_CUSTOM_BOOT_CODE
typedef int  (*usercall_t)(void);


/**
  * this is equivalent to initcalls, but for the purpose of calling
  * userspace code; we provide only 4 levels to allow for some hierarchy;
  * note: this only works with our custom linker scripts
  */

#define __define_usercall(fn, id)					\
        static usercall_t __usercall_##fn __attribute((used))		\
        __attribute__((__section__(".usercall" #id ".user"))) = fn;


#define lvl1_usercall(fn)               __define_usercall(fn, 1)
#define lvl2_usercall(fn)               __define_usercall(fn, 2)
#define lvl3_usercall(fn)               __define_usercall(fn, 3)
#define lvl4_usercall(fn)               __define_usercall(fn, 4)

#endif /* CONFIG_ARCH_CUSTOM_BOOT_CODE */

#endif /* _KERNEL_USER_H_ */
