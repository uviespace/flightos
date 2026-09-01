/**
 * @file include/kernel/xentium.h
 *
 * @ingroup xentium_driver
 *
 * @brief High-level Xentium kernel-loading and processing-task interface.
 *
 * These declarations are the kernel-facing side of `kernel/xentium.c`.
 * Xentium ELF metadata, mailbox commands, and NoC DMA integration are declared
 * in the companion headers in this directory.
 */

#ifndef _KERNEL_XENTIUM_H_
#define _KERNEL_XENTIUM_H_



#include <kernel/init.h>
#include <kernel/elf.h>
#include <kernel/xentium_dev.h>
#include <kernel/xentium_io.h>



/**
 * @brief holds the names, addresses and sizes of the sections in the xentium
 *        kernel so we can reference them individually
 */
struct xen_kern_section {
	char *name;		/*!< section name */
	unsigned long addr;	/*!< section address */
	size_t size;		/*!< section size in bytes */
};


/**
 * @brief tracks a single xentium kernel program
 */
struct xen_kernel {

	unsigned long ep;		/*!< entry point address of the kernel */

	unsigned int align;		/*!< memory alignment of the kernel */

	Elf_Ehdr *ehdr;			/*!< the elf header of the binary */

	size_t size;			/*!< the size of the kernel */

	struct xen_kern_section *sec;	/*!< the (ELF) sections of the kernel */
	size_t num_sec;			/*!< the number of sections */
};



/**
 * @brief add a Xentium kernel program from ELF data
 * @param p: pointer to the ELF binary data of the Xentium kernel
 * @return 0 on success, negative error code on failure
 */
int xentium_kernel_add(void *p);

/**
 * @brief schedule the next Xentium kernel for execution
 */
void xentium_schedule_next(void);

/**
 * @brief submit a processing task to a Xentium kernel
 * @param t: the processing task to submit
 * @return 0 on success, negative error code on failure
 */
int xentium_input_task(struct proc_task *t);

/**
 * @brief retrieve and dispatch completed tasks from Xentium kernels
 */
void xentium_output_tasks(void);

/**
 * @brief configure the output node operator function for Xentium
 * @param op_output: operator function to handle Xentium output
 * @return 0 on success, negative error code on failure
 */
int xentium_config_output_node(op_func_t op_output);

#endif /* _KERNEL_XENTIUM_H_ */
