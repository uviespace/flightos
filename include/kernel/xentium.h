#ifndef _KERNEL_XENTIUM_H_
#define _KERNEL_XENTIUM_H_

#include <kernel/init.h>
#include <kernel/elf.h>

struct xen_module_section {
	char *name;
	unsigned long addr;
	size_t size;
};

struct xen_kernel {

	unsigned long ep;


	int refcnt;

	unsigned int align;

	Elf_Ehdr *ehdr;		/* coincides with start of module image */

	size_t size;


	struct xen_module_section *sec;
	size_t num_sec;
};



int xentium_kernel_load(struct xen_kernel *x, void *p);

#endif /* _KERNEL_XENTIUM_H_ */
