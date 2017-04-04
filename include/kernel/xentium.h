/**
 * @file include/kernel/xentium.h
 */

#ifndef _KERNEL_XENTIUM_H_
#define _KERNEL_XENTIUM_H_



#include <kernel/init.h>
#include <kernel/elf.h>
#include <kernel/xentium_dev.h>
#include <kernel/xentium_io.h>



/**
 * holds the names, addresses and sizes of the sections in the xentium kernel
 * so we can reference them individually
 */

struct xen_kern_section {
	char *name;
	unsigned long addr;
	size_t size;
};


/**
 * tracks a single xentium kernel program
 */

struct xen_kernel {

	unsigned long ep;		/* entry point address of the kernel */

	unsigned int align;		/* memory alignment of the kernel */

	Elf_Ehdr *ehdr;			/* the elf header of the  binary */

	size_t size;			/* the size of the kernel */

	struct xen_kern_section *sec;	/* the (ELF) section of the kernel */
	size_t num_sec;			/* the number of sections */
};



int xentium_kernel_add(void *p);
void xentium_schedule_next(void);
int xentium_input_task(struct proc_task *t);
void xentium_output_tasks(void);
int xentium_config_output_node(op_func_t op_output);

#endif /* _KERNEL_XENTIUM_H_ */
