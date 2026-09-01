/**
 * @file include/kernel/module.h
 *
 * @ingroup elf_loader
 */

#ifndef _KERNEL_MODULE_H_
#define _KERNEL_MODULE_H_

#include <kernel/init.h>
#include <kernel/elf_loader.h>


/**
 * @brief define a module initialization function
 * @param initfunc: the initialization function to register
 *
 * When compiled as a module, creates an alias _module_init to initfunc.
 * When compiled into the kernel, registers initfunc as a device_initcall.
 */
#ifdef MODULE

#define module_init(initfunc)					\
        int _module_init(void) __attribute__((alias(#initfunc)));

/**
 * @brief define a module exit function
 * @param exitfunc: the exit function to register
 *
 * When compiled as a module, creates an alias _module_exit to exitfunc.
 * When compiled into the kernel, registers exitfunc as an exitcall.
 */
#define module_exit(exitfunc)					\
        int _module_exit(void) __attribute__((alias(#exitfunc)));

#else /* MODULE */


#define module_init(initfunc) device_initcall(initfunc);

#define module_exit(exitfunc) __exitcall(exitfunc);


#endif /* MODULE */


/**
 * @brief load a kernel module from an ELF binary
 * @param m: the ELF binary structure to load
 * @param p: pointer to the module image data
 * @return 0 on success, negative error code on failure
 */
int module_load(struct elf_binary *m, void *p);

/**
 * @brief print a list of all loaded modules
 */
void modules_list_loaded(void);

#endif /* _KERNEL_MODULE_H_ */
