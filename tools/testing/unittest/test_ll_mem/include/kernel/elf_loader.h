/**
 * @file include/kernel/elf_loader.h
 *
 * @ingroup elf_loader
 */

#ifndef _KERNEL_ELF_LOADER_H_
#define _KERNEL_ELF_LOADER_H_

#include <kernel/elf.h>


struct elf_section {
	char *name;
	unsigned long addr;
	size_t size;
};


/**
 * this structure contains data for both module and application
 * binaries
 */
struct elf_binary {

	unsigned long pa;
	unsigned long va;

	/* the main base memory block reference for relocatible binaries */
	void *base;

	int (*init)(int argc, char **argv);
	int (*exit)(void);

	int refcnt;

	unsigned int align;

	Elf_Ehdr *ehdr;

	size_t size;

	struct elf_section *sec;
	size_t num_sec;
};


/* implemented in architecture code */
int apply_relocate_add(struct elf_binary *m, Elf_Rela *rel, Elf_Addr sym,
		       const char *sec_name);


struct elf_section *find_elf_sec(const struct elf_binary *m, const char *name);
struct elf_section *find_elf_idx(const struct elf_binary *m, size_t idx);

int setup_elf_binary(struct elf_binary *m);



#endif /* _KERNEL_ELF_LOADER_H_ */
