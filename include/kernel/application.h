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

#define application_init(initfunc)					\
        int _application_init(void) __attribute__((alias(#initfunc)));

#define application_exit(exitfunc)					\
        int _application_exit(void) __attribute__((alias(#exitfunc)));

#else /* APPLICATION */


#define application_init(initfunc) device_initcall(initfunc);

#define application_exit(exitfunc) __exitcall(exitfunc);


#endif /* APPLICATION */


int application_load(void *p, const char *namefmt, int cpu,
		     int argc, char *argv[]);

void applications_list_loaded(void);

#endif /* _KERNEL_APPLICATION_H_ */
