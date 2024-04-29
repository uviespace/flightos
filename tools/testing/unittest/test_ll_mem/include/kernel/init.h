/**
 * @file include/kernel/init.h
 */

#ifndef _KERNEL_INIT_H_
#define _KERNEL_INIT_H_

typedef int  (*initcall_t)(void);
typedef void (*exitcall_t)(void);


/**
 *
 * We piggy-back on the (linux-style) initcall sections that BCC and
 * GCC for the MPPB already have specified in their default linker scripts
 * until we add our own.
 *
 * Since _call_initcalls() is executed by the libgloss bootup-code, use
 * the initcalls in init/main.c to set up our system. Conventions are the same
 * as in linux.
 *
 * They don't have sections for exitcalls, but we don't really need those for
 * now.
 */

#define __define_initcall(fn, id)					\
        static initcall_t __initcall_##fn __attribute((used))		\
        __attribute__((__section__(".initcall" #id ".init"))) = fn;

#if 0
#define __exitcall(fn)							\
        static exitcall_t __exitcall_##fn __attribute((used))		\
        __attribute__((__section__(".exitcall.exit"))) = fn;
#else

#define __exitcall(fn)

#endif

#define core_initcall(fn)               __define_initcall(fn, 1)
#define postcore_initcall(fn)           __define_initcall(fn, 2)
#define arch_initcall(fn)               __define_initcall(fn, 3)
#define subsys_initcall(fn)             __define_initcall(fn, 4)
#define fs_initcall(fn)                 __define_initcall(fn, 5)
#define device_initcall(fn)             __define_initcall(fn, 6)
#define late_initcall(fn)               __define_initcall(fn, 7)


void setup_arch(void);
void main_kernel_loop(void);

#endif /* _KERNEL_INIT_H_ */
