/**
 * @file lib/elf.c
 */


#include <kernel/elf.h>
#include <kernel/string.h>
#include <kernel/printk.h>

#include <asm-generic/swab.h>

#define MSG "ELF: "


/**
 * @brief get the base of the ELF section header(s)
 *
 * @param ehdr an Elf_Ehdr
 *
 * return a Elf_Shdr or NULL if not found
 */

Elf_Shdr *elf_get_shdr(const Elf_Ehdr *ehdr)
{
	Elf_Shdr *shdr = NULL;

	if (ehdr->e_shoff)
		shdr = (Elf_Shdr *) (((char *) ehdr) + ehdr->e_shoff);


	return shdr;
}


/**
 * @brief find section headers marked ALLOC by index
 *
 * @param ehdr an Elf_Ehdr
 *
 *
 * @param offset a offset into the table (to locate multiple identical types)
 *
 * @return section index or 0 if not found
 */

size_t elf_find_shdr_alloc_idx(const Elf_Ehdr *ehdr, const size_t offset)
{
	size_t i;

	Elf_Shdr *shdr;


	shdr = elf_get_shdr(ehdr);
	if (!shdr)
		return 0;


	for (i = offset; i < ehdr->e_shnum; i++) {
		if ((shdr[i].sh_flags & SHF_ALLOC))
			return i;
	}

	return 0;
}


/**
 * @brief return a section header by index
 *
 * @param ehdr an Elf_Ehdr
 *
 * @param idx the section index
 *
 * @return NULL if entry not found, pointer to entry otherwise
 */

Elf_Shdr *elf_get_sec_by_idx(const Elf_Ehdr *ehdr, const size_t idx)
{
	Elf_Shdr *shdr;


	if (idx >= ehdr->e_shnum)
		return NULL;

	shdr = elf_get_shdr(ehdr);
	if (!shdr)
		return NULL;

	return &shdr[idx];
}


/**
 * @brief locate the ".shstrtab" section
 *
 * @param ehdr an Elf_Ehdr
 *
 * @return the section header or NULL if not found
 *
 */

Elf_Shdr *elf_get_sec_shstrtab(const Elf_Ehdr *ehdr)
{
	size_t i;

	char *sh_str;

	Elf_Shdr *shdr;


	shdr = elf_get_shdr(ehdr);
	if (!shdr)
		return NULL;


	for (i = 0; i < ehdr->e_shnum; i++) {

		if (shdr[i].sh_type != SHT_STRTAB)
			continue;

		/* section name index is not within table boundary, skip */
		if (shdr[i].sh_size < shdr[i].sh_name)
			continue;

		sh_str  = (((char *) ehdr) + shdr[i].sh_offset);

		/* it's a string section and the size is ok,
		 * now make sure it is the right one */
		if (!strcmp(sh_str + shdr[i].sh_name, ".shstrtab"))
			return &shdr[i];
	}

	return NULL;
}


/**
 * @brief in-place swap the endianess of an ELF header
 *
 * @param ehdr an Elf_Ehdr
 */

void elf_hdr_endianess_swap(Elf_Ehdr *ehdr)
{
	uint32_t i;
	uint32_t len;

	uint32_t *p;

	Elf_Shdr *shdr;
	Elf_Phdr *phdr;



	/* endianess is a factor from e_type onwards */

	ehdr->e_type	  = swab16(ehdr->e_type);
	ehdr->e_machine	  = swab16(ehdr->e_machine);
	ehdr->e_version	  = swab32(ehdr->e_version);
	ehdr->e_entry	  = swab32(ehdr->e_entry);
	ehdr->e_phoff	  = swab32(ehdr->e_phoff);
	ehdr->e_shoff	  = swab32(ehdr->e_shoff);
	ehdr->e_flags	  = swab32(ehdr->e_flags);
	ehdr->e_ehsize	  = swab16(ehdr->e_ehsize);
	ehdr->e_phentsize = swab16(ehdr->e_phentsize);
	ehdr->e_phnum	  = swab16(ehdr->e_phnum);
	ehdr->e_shentsize = swab16(ehdr->e_shentsize);
	ehdr->e_shnum	  = swab16(ehdr->e_shnum);
	ehdr->e_shstrndx  = swab16(ehdr->e_shstrndx);

	/* everything after is in 32 bit words */

	phdr = (Elf_Phdr *) ((char *) ehdr + ehdr->e_phoff);

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr[i].p_type   = swab32(phdr[i].p_type);
		phdr[i].p_flags  = swab32(phdr[i].p_flags);
		phdr[i].p_offset = swab32(phdr[i].p_offset);
		phdr[i].p_vaddr  = swab32(phdr[i].p_vaddr);
		phdr[i].p_paddr  = swab32(phdr[i].p_paddr);
		phdr[i].p_filesz = swab32(phdr[i].p_filesz);
		phdr[i].p_memsz  = swab32(phdr[i].p_memsz);
		phdr[i].p_align  = swab32(phdr[i].p_align);
	}

	shdr = (Elf_Shdr *) ((char *) ehdr + ehdr->e_shoff);

	for (i = 0; i < ehdr->e_shnum; i++) {
		shdr[i].sh_name      = swab32(shdr[i].sh_name);
		shdr[i].sh_type      = swab32(shdr[i].sh_type);
		shdr[i].sh_flags     = swab32(shdr[i].sh_flags);
		shdr[i].sh_addr      = swab32(shdr[i].sh_addr);
		shdr[i].sh_offset    = swab32(shdr[i].sh_offset);
		shdr[i].sh_size      = swab32(shdr[i].sh_size);
		shdr[i].sh_link      = swab32(shdr[i].sh_link);
		shdr[i].sh_info      = swab32(shdr[i].sh_info);
		shdr[i].sh_addralign = swab32(shdr[i].sh_addralign);
		shdr[i].sh_entsize   = swab32(shdr[i].sh_entsize);
	}
}

/**
 * @brief get the address of the section header string table
 *
 * @param ehdr an Elf_Ehdr
 *
 * @return the address of the section header string table or NULL if not found
 */

char *elf_get_shstrtab(const Elf_Ehdr *ehdr)
{
	char *sh_str;

	Elf_Shdr *shdr;


	shdr = elf_get_sec_shstrtab(ehdr);
	if (!shdr)
		return NULL;


	sh_str = (((char *) ehdr) + shdr->sh_offset);

	return sh_str;
}


/**
 * @brief get an entry in the .shstrtab
 *
 * @param ehdr an Elf_Ehdr
 *
 * @param idx the section index
 *
 * @return pointer to the start of the string or NULL if not found
 */

char *elf_get_shstrtab_str(const Elf_Ehdr *ehdr, size_t idx)
{
	char *sh_str;

	Elf_Shdr *shdr;



	if (idx >= ehdr->e_shnum)
		return NULL;

	shdr = elf_get_shdr(ehdr);
	if (!shdr)
		return NULL;

	sh_str = elf_get_shstrtab(ehdr);
	if (!sh_str)
		return NULL;


	return sh_str + shdr[idx].sh_name;
}



/**
 * @brief find an elf section by name
 *
 * @return section index or 0 if not found
 */

Elf_Shdr *elf_find_sec(const Elf_Ehdr *ehdr, const char *name)
{
	size_t i;

	char *sh_str;

	Elf_Shdr *shdr;


	shdr = elf_get_sec_shstrtab(ehdr);
	if (!shdr)
		return NULL;

	sh_str  = ((char *) ehdr + shdr->sh_offset);

	for (i = 1; i < ehdr->e_shnum; i++) {
		if (!strcmp(sh_str + shdr[i].sh_name, name))
			return &shdr[i];
	}

	return NULL;
}


/**
 * @brief get the address of the string table
 *
 * @param ehdr an Elf_Ehdr
 *
 * @return the address of the string table or NULL if not found
 */

char *elf_get_strtab(const Elf_Ehdr *ehdr)
{
	char *sh_str;

	Elf_Shdr *shdr;


	shdr = elf_find_sec(ehdr, ".strtab");
	if (!shdr)
		return NULL;


	sh_str = (((char *) ehdr) + shdr->sh_offset);

	return sh_str;
}


/**
 * @brief get an entry in the .strtab
 *
 * @param ehdr an Elf_Ehdr
 *
 * @param idx the entry index
 *
 * @return pointer to the start of the string or NULL if not found
 */

char *elf_get_strtab_str(const Elf_Ehdr *ehdr, size_t idx)
{
	char *str;

	Elf_Shdr *shdr;



	if (idx >= ehdr->e_shnum)
		return NULL;

	shdr = elf_get_shdr(ehdr);
	if (!shdr)
		return NULL;

	str = elf_get_strtab(ehdr);
	if (!str)
		return NULL;


	return str + shdr[idx].sh_name;
}


/**
 * @brief get the address of the dynamic string table
 *
 * @param ehdr an Elf_Ehdr
 *
 * @return the address of the string table or NULL if not found
 */

char *elf_get_dynstr(const Elf_Ehdr *ehdr)
{
	char *dyn_str;

	Elf_Shdr *shdr;


	shdr = elf_find_sec(ehdr, ".dynstr");
	if (!shdr)
		return NULL;


	dyn_str = (((char *) ehdr) + shdr->sh_offset);

	return dyn_str;
}


/**
 * @brief get an entry in the .strtab
 *
 * @param ehdr an Elf_Ehdr
 *
 * @param idx the entry index
 *
 * @return pointer to the start of the string or NULL if not found
 */

char *elf_get_dynstr_str(const Elf_Ehdr *ehdr, size_t idx)
{
	char *sh_str;

	Elf_Shdr *shdr;



	if (idx >= ehdr->e_shnum)
		return NULL;

	shdr = elf_get_shdr(ehdr);
	if (!shdr)
		return NULL;

	sh_str = elf_get_dynstr(ehdr);
	if (!sh_str)
		return NULL;


	return sh_str + shdr[idx].sh_name;
}


/**
 * @brief get the name of a symbol in .symtab with a given index
 *
 * @param ehdr an Elf_Ehdr
 *
 * @param idx the symbol index
 *
 * @return pointer to the start of the string or NULL if not found
 */

char *elf_get_symbol_str(const Elf_Ehdr *ehdr, size_t idx)
{
	Elf_Shdr *symtab;
	Elf_Sym *symbols;


	symtab = elf_find_sec(ehdr, ".symtab");
	if (!symtab)
		return NULL;

	symbols = (Elf_Sym *) (((char *) ehdr) + symtab->sh_offset);

	if (idx < symtab->sh_size / symtab->sh_entsize)
		return elf_get_strtab(ehdr) + symbols[idx].st_name;

	return NULL;
}


/**
 * @brief get the value of a symbol
 *
 * @param ehdr an Elf_Ehdr
 *
 * @param name the name of the symbol
 *
 * @param[out] value the value of the symbol
 *
 * @return 1 if symbol has been found, 0 otherwise
 */

unsigned long elf_get_symbol_value(const Elf_Ehdr *ehdr,
				   const char *name, unsigned long *value)

{
	unsigned int i;
	unsigned int idx;
	size_t sym_cnt;

	Elf_Shdr *symtab;
	Elf_Sym *symbols;


	symtab = elf_find_sec(ehdr, ".symtab");

	if (!symtab) {
		pr_debug(MSG "WARN: no .symtab section found\n");
		return -1;
	}

	if (symtab->sh_entsize != sizeof(Elf_Sym)) {
		pr_debug(MSG "Error %d != %ld\n",
			 sizeof(Elf_Sym), symtab->sh_entsize);
		return -1;
	}

	symbols = (Elf_Sym *) (((char *) ehdr) + symtab->sh_offset);

	sym_cnt = symtab->sh_size / symtab->sh_entsize;

	for (i = 0; i < sym_cnt; i++) {
		if(!strcmp(elf_get_symbol_str(ehdr, i), name)) {
			(*value) =  symbols[i].st_value;
			return 1;
		}
	}

	return 0;
}


/**
 * @brief get the ELF type of a symbol
 *
 * @param ehdr an Elf_Ehdr
 *
 * @param name the name of the symbol
 *
 * @return the symbol type or 0 on error or if not found
 */

unsigned long elf_get_symbol_type(const Elf_Ehdr *ehdr, const char *name)
{
	unsigned int i;
	size_t sym_cnt;

	Elf_Shdr *symtab;
	Elf_Sym *symbols;


	symtab = elf_find_sec(ehdr, ".symtab");

	if (!symtab) {
		pr_debug(MSG "WARN: no .symtab section found\n");
		return 0;
	}

	if (symtab->sh_entsize != sizeof(Elf_Sym)) {
		pr_debug("Error %d != %ld\n", sizeof(Elf_Sym), symtab->sh_entsize);
		return 0;
	}

	symbols = (Elf_Sym *) (((char *) ehdr) + symtab->sh_offset);

	sym_cnt = symtab->sh_size / symtab->sh_entsize;

	for (i = 0; i < sym_cnt; i++) {
		if(!strcmp(elf_get_symbol_str(ehdr, i), name))
			return ELF_ST_TYPE(symbols[i].st_info);
	}

	return 0;
}


/**
 * @brief find an elf section index by type
 *
 * @param ehdr an Elf_Ehdr
 *
 * @param sh_type the section type to look for
 *
 * @param offset a offset into the table (to locate multiple identical types)
 *
 * @return section index or 0 if not found
 *
 */

size_t elf_find_sec_idx_by_type(const Elf_Ehdr *ehdr,
				const uint32_t sh_type,
				const size_t offset)
{
	size_t i;

	Elf_Shdr *shdr;


	for (i = offset; i < ehdr->e_shnum; i++) {
		shdr = elf_get_sec_by_idx(ehdr, i);
		if (!shdr)
			return 0;
		if (sh_type == shdr->sh_type)
			return i;
	}

	return 0;
}


/**
 * @brief get number of sections marked ALLOC with size > 0
 *
 * @param ehdr an Elf_Ehdr
 *
 * @return number of ALLOC sections
 */

size_t elf_get_num_alloc_sections(const Elf_Ehdr *ehdr)
{
	size_t i;
	size_t cnt = 0;

	Elf_Shdr *shdr;


	for (i = 0; i <ehdr->e_shnum; i++) {
		shdr = elf_get_sec_by_idx(ehdr, i);
		if ((shdr->sh_flags & SHF_ALLOC))
			if (shdr->sh_size)
				cnt++;
	}

	return cnt;
}


/**
 * @brief get the number of entries in a dynamic section
 *
 * @param ehdr an Elf_Ehdr
 *
 * @return the number of entries in a dynamic section
 */

size_t elf_get_num_dyn_entries(const Elf_Ehdr *ehdr)
{
	Elf_Shdr *shdr;


	shdr = elf_find_sec(ehdr, ".dyn");
	if (!shdr)
		return 0;


	return shdr->sh_size / sizeof(Elf_Dyn);
}


/**
 * @brief find a dynamic entry by tag
 *
 * @param offset a offset into the table (to locate multiple identical types)
 *
 * @return NULL if entry not found, pointer to entry otherwise
 *
 */
Elf_Dyn *elf_find_dyn(const Elf_Ehdr *ehdr,
		      const unsigned long d_tag,
		      const unsigned int offset)
{
	size_t i;

	Elf_Shdr *shdr;

	Elf_Dyn *dyn;


	shdr = elf_find_sec(ehdr, ".dyn");

	dyn = (Elf_Dyn *) (((char *) ehdr) + shdr->sh_offset);

	for (i = offset; i < elf_get_num_dyn_entries(ehdr); i++)
		if (d_tag == dyn[i].d_tag)
			return &dyn[i];

	return NULL;
}




/**
 * @brief dump the contents of .strtab
 */

void elf_dump_strtab(const Elf_Ehdr *ehdr)
{
	size_t i = 0;

	char *s;


	printk(MSG "\n.strtab:\n"
	       "============================\n"
	       "\t[OFF]\t[STR]\n");

	while((s = elf_get_strtab_str(ehdr, i++)))
		printk(MSG "\t[%d]\t%s\n", i, s);

	printk(MSG "\n\n");
}


/**
 * @brief dump the contents of .symtab
 */

void elf_dump_symtab(const Elf_Ehdr *ehdr)
{
	Elf_Shdr *symtab;
	Elf_Sym *symbols;

	size_t sym_cnt;
	unsigned int idx;
	unsigned int i;



	symtab = elf_find_sec(ehdr, ".symtab");

	if (!symtab) {
		printk(MSG "WARN: no .symtab section found\n");
		return;
	}


	if (symtab->sh_entsize != sizeof(Elf_Sym)) {
		printk(MSG "Error %d != %ld\n", sizeof(Elf_Sym), symtab->sh_entsize);
		return;
	}

	symbols = (Elf_Sym *) (((char *) ehdr) + symtab->sh_offset);

	sym_cnt = symtab->sh_size / symtab->sh_entsize;


	printk(MSG "\n"
	       MSG ".symtab contains %d entries\n"
	       MSG "============================\n"
	       MSG "\t[NUM]\t[VALUE]\t\t\t[SIZE]\t[TYPE]\t[NAME]\n", sym_cnt);


	for (i = 0; i < sym_cnt; i++) {

		printk(MSG "\t%d\t%016lx\t%4ld",
		       i,
		       symbols[i].st_value,
		       symbols[i].st_size);

		switch (ELF_ST_TYPE(symbols[i].st_info)) {
		case STT_NOTYPE  :
			printk("\tNOTYPE "); break;
		case STT_OBJECT  :
			printk("\tOBJECT "); break;
		case STT_FUNC    :
			printk("\tFUNC   "); break;
		case STT_SECTION :
			printk("\tSECTION"); break;
		case STT_FILE    :
			printk("\tFILE   "); break;
		case STT_COMMON  :
			printk("\tCOMMON "); break;
		case STT_TLS     :
			printk("\tTLS    "); break;
		default:
			printk("\tUNKNOWN"); break;
		}

		printk("\t%-10s\n", elf_get_symbol_str(ehdr, i));

	}
}

/**
 * @brief dump the name of all elf sections
 */

void elf_dump_sections(const Elf_Ehdr *ehdr)
{
	size_t i;

	Elf_Shdr *shdr;


	printk(MSG "SECTIONS:\n"
	       MSG "============================\n"
	       MSG "\t[NUM]\t[NAME]\t\t\t[TYPE]\t\t[SIZE]\t[ENTSZ]\t[FLAGS]\n"
	      );


	for (i = 0; i < ehdr->e_shnum; i++) {

		shdr = elf_get_sec_by_idx(ehdr, i);
		printk(MSG "\t%d\t%-20s", i, elf_get_shstrtab_str(ehdr, i));

		switch (shdr->sh_type) {
		case SHT_NULL:
			printk("%-10s", "\tNULL");
			break;
		case SHT_PROGBITS:
			printk("%-10s", "\tPROGBITS");
			break;
		case SHT_SYMTAB:
			printk("%-10s", "\tSYMTAB");
			break;
		case SHT_STRTAB:
			printk("%-10s", "\tSTRTAB");
			break;
		case SHT_RELA  :
			printk("%-10s", "\tRELA");
			break;
		case SHT_HASH  :
			printk("%-10s", "\tHASH");
			break;
		case SHT_DYNAMIC:
			printk("%-10s", "\tDYNAMIC");
			break;
		case SHT_NOTE  :
			printk("%-10s", "\tNOTE");
			break;
		case SHT_NOBITS:
			printk("%-10s", "\tNOBITS");
			break;
		case SHT_REL   :
			printk("%-10s", "\tREL");
			break;
		case SHT_SHLIB :
			printk("%-10s", "\tSHLIB");
			break;
		case SHT_DYNSYM:
			printk("%-10s", "\tDYNSYM");
			break;
		case SHT_NUM   :
			printk("%-10s", "\tNUM");
			break;
		default:
			printk("%-10s", "\tOTHER");
			break;
		}

		printk("\t%ld\t%ld\t", shdr->sh_size, shdr->sh_entsize);

		if (shdr->sh_flags & SHF_WRITE)
			printk("WRITE ");
		if (shdr->sh_flags & SHF_ALLOC)
			printk("ALLOC ");
		if (shdr->sh_flags & SHF_EXECINSTR)
			printk("EXECINSTR ");
		if (shdr->sh_flags & SHF_TLS)
			printk("TLS ");
		if (shdr->sh_flags & SHF_MASKPROC)
			printk("MASKPROC ");

		printk(MSG "\n");

	}

}

