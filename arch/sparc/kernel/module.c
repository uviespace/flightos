/**
 * @file arch/sparc/kernel/module.c
 *
 * @ingroup sparc
 * @brief implements architecture-specific @ref kernel_module interfaces
 */

#include <kernel/module.h>
#include <kernel/printk.h>
#include <kernel/err.h>


/**
 * @brief apply relocation + addend
 *
 * @param m an ELF module
 * @param rel an ELF relocation entry
 * @param sym the address of the target symbol
 *
 * return 0 on success
 */

int apply_relocate_add(struct elf_module *m, Elf_Rela *rel, Elf_Addr sym)
{
	Elf_Addr rsym;

	uint8_t  *loc8;
	uint32_t *loc32;

	struct module_section *text;



	if (!m)
		return -EINVAL;

	if (!rel)
		return -EINVAL;

	if (!sym)
		return -EINVAL;



	text = find_mod_sec(m, ".text");

	loc8  = (uint8_t  *) (text->addr + rel->r_offset);
	loc32 = (uint32_t *) loc8;


	/* the symbol address it is referring to */
	rsym = sym + rel->r_addend;


	/* SPARC relocations are annoying, these are just the most common ones,
	 * add as needed
	 * http://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-54839.html
	 */

	switch (ELF_R_TYPE(rel->r_info) & 0xff) {
	case R_SPARC_DISP32:	/* S + A - P */
		pr_debug("\tREL type R_SPARC_DISP32\n");
		rsym -= (Elf_Addr) loc8;
		(*loc32) = rsym;
		break;

	case R_SPARC_32:	/* S + A */
	case R_SPARC_UA32:	/* L + A */
		pr_debug("\tREL type R_SPARC_(UA)32\n");
		loc8[0] = rsym >> 24;
		loc8[1] = rsym >> 16;
		loc8[2] = rsym >>  8;
		loc8[3] = rsym >>  0;
		break;

	case R_SPARC_WDISP30:	/* (S + A - P) >> 2 */
		pr_debug("\tREL type R_SPARC_WDISP30\n");
		rsym -= (Elf_Addr) loc8;
		(*loc32) = ((*loc32) & ~(((1 << 30)) - 1)) |
			((rsym >> 2) &   ((1 << 30)  - 1));
		break;

	case R_SPARC_WDISP22:
		pr_debug("\tREL type R_SPARC_WDISP22\n");
		rsym -= (Elf_Addr) loc8;
		(*loc32) = ((*loc32) & ~(((1 << 22)) - 1)) |
			((rsym >> 2) &   ((1 << 22)  - 1));
		break;

	case R_SPARC_HI22:
		pr_debug("\tREL type R_SPARC_HI22\n");
		(*loc32) = ((*loc32) & ~(((1 << 22)) - 1)) |
			((rsym >> 10) &   ((1 << 22)  - 1));
		break;

	case R_SPARC_LO10:
		pr_debug("\tREL type R_SPARC_LO10\n");
		(*loc32) = ((*loc32) & ~(((1 << 10)) - 1)) |
			(rsym &   ((1 << 10)  - 1));
		break;

	default:
		pr_err("\tUnsupported relocation type: %x\n",
		       (ELF_R_TYPE(rel->r_info) & 0xff));
		return -ENOEXEC;
	}

	return 0;
}
