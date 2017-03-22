#ifndef _KERNEL_XENTIUM_H_
#define _KERNEL_XENTIUM_H_

#include <kernel/init.h>
#include <kernel/elf.h>
#include <data_proc_net.h>

/* this structure is used in xentium kernels to define their parameters
 * and capabilities
 */
struct xen_kernel_cfg {
	char *name;
	unsigned long op_code;
	unsigned long crit_buf_lvl;
};


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



int xentium_kernel_add(void *p);
void xentium_schedule_next(void);
void xentium_input_task(struct proc_task *t);
int xentium_config_output_node(op_func_t op_output);

#endif /* _KERNEL_XENTIUM_H_ */
