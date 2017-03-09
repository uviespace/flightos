/**
 * @file include/kernel/init.h
 */

#ifndef _KERNEL_INIT_H_
#define _KERNEL_INIT_H_

typedef int (*initcall_t)(void);

/* TODO: initcalls on startup for compiled-in modules */
#define define_initcall(fn) \
        static initcall_t _initcall_##fn __attribute__((used)) \
        __attribute__((__section__(".initcall"))) = fn;

void setup_arch(void);

#endif /* _KERNEL_INIT_H_ */
