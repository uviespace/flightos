/**
 * @file include/kernel/module.h
 *
 * @ingroup elf_loader
 */

#ifndef _KERNEL_MODULE_H_
#define _KERNEL_MODULE_H_

#include <kernel/init.h>
#include <kernel/elf_loader.h>


#ifdef MODULE

#define module_init(initfunc)					\
        int _module_init(void) __attribute__((alias(#initfunc)));

#define module_exit(exitfunc)					\
        int _module_exit(void) __attribute__((alias(#exitfunc)));

#else /* MODULE */


#define module_init(initfunc) device_initcall(initfunc);

#define module_exit(exitfunc) __exitcall(exitfunc);


#endif /* MODULE */


int module_load(struct elf_binary *m, void *p);

void modules_list_loaded(void);

#endif /* _KERNEL_MODULE_H_ */
