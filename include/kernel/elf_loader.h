/**
 * @file include/kernel/elf_loader.h
 *
 * @ingroup elf_loader
 */

#ifndef _KERNEL_ELF_LOADER_H_
#define _KERNEL_ELF_LOADER_H_

#include <kernel/elf.h>


/**
 * @brief ELF section descriptor
 */
struct elf_section {
	char *name;			/*!< section name */
	unsigned long addr;		/*!< virtual address of the section */
	size_t size;			/*!< size of the section in bytes */
};


/**
 * @brief this structure contains data for both module and application
 *        binaries
 */
struct elf_binary {

	unsigned long pa;	/*!< physical address of the binary */
	unsigned long va;	/*!< virtual address of the binary */

	void *base;		/*!< main base memory block for relocatable binaries */

	int (*init)(int argc, char **argv);	/*!< init function from binary */
	int (*exit)(void);			/*!< exit function from binary */

	int refcnt;		/*!< reference count */

	unsigned int align;	/*!< memory alignment requirement */

	Elf_Ehdr *ehdr;	/*!< ELF header */

	size_t size;		/*!< total size of the binary in memory */

	struct elf_section *sec;	/*!< array of ELF sections */
	size_t num_sec;			/*!< number of sections */
};


/**
 * @brief apply a relocation with addend (architecture-specific)
 * @param m: the ELF binary being relocated
 * @param rel: the relocation entry
 * @param sym: the symbol value for the relocation
 * @param sec_name: name of the section being relocated
 * @return 0 on success, negative error code on failure
 */
int apply_relocate_add(struct elf_binary *m, Elf_Rela *rel, Elf_Addr sym,
		       const char *sec_name);


/**
 * @brief find an ELF section by name
 * @param m: the ELF binary to search
 * @param name: section name to find
 * @return pointer to the section, or NULL if not found
 */
struct elf_section *find_elf_sec(const struct elf_binary *m, const char *name);

/**
 * @brief find an ELF section by index
 * @param m: the ELF binary to search
 * @param idx: section index
 * @return pointer to the section, or NULL if not found
 */
struct elf_section *find_elf_idx(const struct elf_binary *m, size_t idx);

/**
 * @brief set up an ELF binary (parse headers, apply relocations)
 * @param m: the ELF binary to set up
 * @return 0 on success, negative error code on failure
 */
int setup_elf_binary(struct elf_binary *m);



#endif /* _KERNEL_ELF_LOADER_H_ */
