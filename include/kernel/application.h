/**
 * @file include/kernel/application.h
 *
 * @ingroup elf_loader
 */

#ifndef _KERNEL_APPLICATION_H_
#define _KERNEL_APPLICATION_H_

#include <kernel/init.h>
#include <kernel/elf_loader.h>


#ifdef APPLICATION

/**
 * @brief define an application initialization function
 * @param initfunc: the initialization function to register
 *
 * When compiled as an application, creates an alias _application_init.
 * When compiled into the kernel, registers initfunc as a device_initcall.
 */
#define application_init(initfunc)					\
        int _application_init(void) __attribute__((alias(#initfunc)));

/**
 * @brief define an application exit function
 * @param exitfunc: the exit function to register
 *
 * When compiled as an application, creates an alias _application_exit.
 * When compiled into the kernel, registers exitfunc as an exitcall.
 */
#define application_exit(exitfunc)					\
        int _application_exit(void) __attribute__((alias(#exitfunc)));

#else /* APPLICATION */


#define application_init(initfunc) device_initcall(initfunc);

#define application_exit(exitfunc) __exitcall(exitfunc);


#endif /* APPLICATION */


/**
 * @brief load and start an application from ELF binary data
 * @param p: pointer to the ELF binary data
 * @param namefmt: printf-style format string for the application name
 * @param cpu: CPU id to bind the application to
 * @param argc: argument count
 * @param argv: argument vector
 * @return 0 on success, negative error code on failure
 */
int application_load(void *p, const char *namefmt, int cpu,
		     int argc, char *argv[]);

/**
 * @brief print a list of all loaded applications
 */
void applications_list_loaded(void);

#endif /* _KERNEL_APPLICATION_H_ */
