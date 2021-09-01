/**
 * @file arch/sparc/kernel/elf.c
 *
 * @ingroup sparc
 *
 * @brief implements the architecture-specific ELF interface
 *
 */

#include <kernel/printk.h>
#include <kernel/kernel_levels.h>
#include <kernel/elf.h>


/**
 * @brief check if the ELF file can be used by us 
 */

int elf_header_check(Elf_Ehdr *ehdr)
{
	if (ehdr->e_shentsize != sizeof(Elf_Shdr))
		return -1;

	if (!(ehdr->e_ident[EI_MAG0] == ELFMAG0))
		return -1;

	if (!(ehdr->e_ident[EI_MAG1] == ELFMAG1))
		return -1;

	if (!(ehdr->e_ident[EI_MAG2] == ELFMAG2))
		return -1;

	if (!(ehdr->e_ident[EI_MAG3] == ELFMAG3))
		return -1;

	if (!(ehdr->e_ident[EI_CLASS] == ELFCLASS32))
		if (!(ehdr->e_ident[EI_CLASS] == ELFCLASS64))
			return -1;

	if (!elf_check_endian(ehdr))
		return -1;

	if (!(ehdr->e_ident[EI_VERSION] == EV_CURRENT))
		return -1;

	/* .o files only */
	if (!(ehdr->e_type == ET_REL)) {
		pr_err("SPARC ELF: only relocatible code is supported\n");
		return -1;
	}

	if(!elf_check_arch(ehdr))
		return -1;

	if (!(ehdr->e_version == EV_CURRENT))
		return -1;

	return 0;
}
