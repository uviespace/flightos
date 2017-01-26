#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/printk.h>
#include <kernel/err.h>
#include <kernel/module.h>
#include <kernel/ksym.h>


/* XXX quick and dirty malloc() standin */
#include <page.h>
void *malloc(size_t size) {
	return page_alloc();
}


/**
 * @brief find section headers marked ALLOC by index
 *
 * @param offset a offset into the table (to locate multiple identical types)
 *
 * @return section index or 0 if not found
 */

static unsigned int find_shdr_alloc(const struct elf_module *m,
				    const unsigned int offset)
{
	size_t i;


	for (i = offset; i < m->ehdr->e_shnum; i++) {
		if ((m->shdr[i].sh_flags & SHF_ALLOC))
			return i;
	}

	return 0;
}



/**
 * @brief get number of sections marked ALLOC with size > 0
 *
 * @return number of ALLOC sections
 */

static unsigned int get_num_alloc_sections(const struct elf_module *m)
{
	size_t i;
	size_t cnt = 0;


	for (i = 0; i < m->ehdr->e_shnum; i++) {
		if ((m->shdr[i].sh_flags & SHF_ALLOC))
			if (m->shdr[i].sh_size)
				cnt++;
	}

	return cnt;
}


/**
 * @brief get the number of entries in a dynamic section
 *
 * @return the number of entries in a dynamic section
 */

static unsigned int get_num_dyn_entries(const struct elf_module *m)
{
	return m->dyn_size / sizeof(Elf_Dyn);
}

/**
 * @brief find a dynamic entry by tag
 *
 * @param offset a offset into the table (to locate multiple identical types)
 *
 * @return NULL if entry not found, pointer to entry otherwise
 *
 */
__attribute__((unused))
static Elf_Dyn *find_dyn(const struct elf_module *m,
			 const typeof(m->dyn->d_tag) d_tag,
			 const unsigned int offset)
{
	size_t i;


	for (i = offset; i < get_num_dyn_entries(m); i++)
		if (d_tag == m->dyn[i].d_tag)
			return &m->dyn[i];

	return NULL;
}


/**
 * @brief return a section header by index
 *
 * @return NULL if entry not found, pointer to entry otherwise
 *
 */

static Elf_Shdr *get_sec(const struct elf_module *m, unsigned int idx)
{
	if (idx < m->ehdr->e_shnum)
		return &m->shdr[idx];

	return NULL;
}


/**
 * @brief find find an elf section by type
 *
 * @param offset a offset into the table (to locate multiple identical types)
 *
 * @return section index or 0 if not found
 *
 */

static unsigned int find_sec_type(const struct elf_module *m,
				  const typeof(m->shdr->sh_type) sh_type,
				  const unsigned int offset)
{
	size_t i;


	for (i = offset; i < m->ehdr->e_shnum; i++) {
		if (sh_type == m->shdr[i].sh_type)
			return i;
	}

	return 0;
}



/**
 * @brief find an elf section by name
 *
 * @return section index or 0 if not found
 */

static unsigned int find_sec(const struct elf_module *m, const char *name)
{
	size_t i;


	for (i = 1; i < m->ehdr->e_shnum; i++) {
		if (!strcmp(m->sh_str + m->shdr[i].sh_name, name))
			return i;
	}

	return 0;
}

/**
 * @brief get an entry in the .shstrtab
 *
 * @return pointer to the start of the string or NULL if not found
 */

static char *get_shstrtab_str(const struct elf_module *m, unsigned int idx)
{
	if (idx < m->sh_size)
		return (m->sh_str + m->shdr[idx].sh_name);

	return NULL;
}

/**
 * @brief get an entry in the .strtab
 *
 * @return pointer to the start of the string or NULL if not found
 */

__attribute__((unused))
static char *get_strtab_str(const struct elf_module *m, unsigned int idx)
{
	if (idx < m->str_size)
		return (m->str + idx);

	return NULL;
}

	
/**
 * @brief get the name of a symbol in .symtab with a given index
 *
 * @return pointer to the start of the string or NULL if not found
 */

static char *get_symbol_str(const struct elf_module *m, unsigned int idx)
{
	Elf_Shdr *symtab;
	Elf_Sym *symbols;


	symtab = &m->shdr[find_sec(m, ".symtab")];

	//symbols = (Elf_Sym *) (m->pa + symtab->sh_offset);
	symbols = (Elf_Sym *) ((unsigned long) m->ehdr + symtab->sh_offset);
	
	if (idx < symtab->sh_size / symtab->sh_entsize)
		return m->str + symbols[idx].st_name;
	
	return NULL;
}



/**
 * @brief find module section by name
 *
 * @return module section structure pointer or NULL if not found 
 */

struct module_section *find_mod_sec(const struct elf_module *m,
				    const char *name) 

{
	size_t i;


	for (i = 0; i < m->num_sec; i++) {
		if (!strcmp(m->sec[i].name, name))
			return &m->sec[i];
	}


	return NULL;
}


/**
 * @brief dump the name of all elf sections
 */

void dump_sections(const struct elf_module *m)
{
	size_t i;


	printk("\nSECTIONS:\n"
	       "============================\n"
	       "\t[NUM]\t[NAME]\t\t\t[TYPE]\t\t[SIZE]\t[ENTSZ]\t[FLAGS]\n");


	for (i = 0; i < m->ehdr->e_shnum; i++) {

		printk("\t%d\t%-20s", i, m->sh_str + m->shdr[i].sh_name);

		switch (m->shdr[i].sh_type) {
		case SHT_NULL:
			printk("%-10s", "\tNULL"); break;
		case SHT_PROGBITS:
			printk("%-10s", "\tPROGBITS");
			break;
		case SHT_SYMTAB:
			printk("%-10s", "\tSYMTAB"); break;
		case SHT_STRTAB:
			printk("%-10s", "\tSTRTAB"); break;
		case SHT_RELA  :
			printk("%-10s", "\tRELA"); break;
		case SHT_HASH  :
			printk("%-10s", "\tHASH"); break;
		case SHT_DYNAMIC:
			printk("%-10s", "\tDYNAMIC"); break;
		case SHT_NOTE  :
			printk("%-10s", "\tNOTE"); break;
		case SHT_NOBITS:
			printk("%-10s", "\tNOBITS"); break;
		case SHT_REL   :
			printk("%-10s", "\tREL"); break;
		case SHT_SHLIB :
			printk("%-10s", "\tSHLIB"); break;
		case SHT_DYNSYM:
			printk("%-10s", "\tDYNSYM"); break;
		case SHT_NUM   :
			printk("%-10s", "\tNUM"); break;
		default:
			printk("%-10s", "\tOTHER"); break;
		}

		printk("\t%ld\t%ld\t", m->shdr[i].sh_size, m->shdr[i].sh_entsize);

		if (m->shdr[i].sh_flags & SHF_WRITE)
			printk("WRITE ");
		if (m->shdr[i].sh_flags & SHF_ALLOC)
			printk("ALLOC ");
		if (m->shdr[i].sh_flags & SHF_EXECINSTR)
			printk("EXECINSTR ");
		if (m->shdr[i].sh_flags & SHF_TLS)
			printk("TLS ");
		if (m->shdr[i].sh_flags & SHF_MASKPROC)
			printk("MASKPROC ");

		printk("\n");

	}

}


/**
 * @brief get the type of a symbol 
 *
 * @return 1 if symbol has been found, 0 otherwise
 */

static unsigned long get_symbol_type(const struct elf_module *m,
				     const char *name) 

{
	unsigned int i;
	unsigned int idx;
	size_t sym_cnt;
	
	Elf_Shdr *symtab;
	Elf_Sym *symbols;


	idx = find_sec(m, ".symtab");

	if (!idx) {
		printf("WARN: no .symtab section found\n");
		return -1;
	}

	symtab = &m->shdr[idx];

	if (symtab->sh_entsize != sizeof(Elf_Sym)) {
		printk("Error %d != %ld\n", sizeof(Elf_Sym), symtab->sh_entsize);
		return -1;
	}
	
	symbols = (Elf_Sym *) ((unsigned long) m->ehdr + symtab->sh_offset);
	
	sym_cnt = symtab->sh_size / symtab->sh_entsize;
	
	for (i = 0; i < sym_cnt; i++) {
		if(!strcmp(get_symbol_str(m, i), name)) {
			return ELF_ST_TYPE(symbols[i].st_info);
		}
	}

	return 0;
}



/**
 * @brief get the value of a symbol 
 *
 * @return 1 if symbol has been found, 0 otherwise
 */

static unsigned long get_symbol_value(const struct elf_module *m,
				      const char *name, unsigned long *value) 

{
	unsigned int i;
	unsigned int idx;
	size_t sym_cnt;
	
	Elf_Shdr *symtab;
	Elf_Sym *symbols;


	idx = find_sec(m, ".symtab");

	if (!idx) {
		printf("WARN: no .symtab section found\n");
		return -1;
	}

	symtab = &m->shdr[idx];

	if (symtab->sh_entsize != sizeof(Elf_Sym)) {
		printf("Error %d != %ld\n", sizeof(Elf_Sym), symtab->sh_entsize);
		return -1;
	}
	
	symbols = (Elf_Sym *) ((unsigned long) m->ehdr + symtab->sh_offset);
	
	sym_cnt = symtab->sh_size / symtab->sh_entsize;
	
	for (i = 0; i < sym_cnt; i++) {
		if(!strcmp(get_symbol_str(m, i), name)) {
			(*value) =  symbols[i].st_value;
			return 1;
		}
	}

	return 0;
}

/**
 * @brief dump the contents of .symtab
 */

static int dump_symtab(struct elf_module *m)
{
	Elf_Shdr *symtab;
	Elf_Sym *symbols;

	size_t sym_cnt;
	unsigned int idx;
	unsigned int i;



	idx = find_sec(m, ".symtab");

	if (!idx) {
		printf("WARN: no .symtab section found\n");
		return -1;
	}

	symtab = &m->shdr[idx];

	if (symtab->sh_entsize != sizeof(Elf_Sym)) {
		printf("Error %d != %ld\n", sizeof(Elf_Sym), symtab->sh_entsize);
		return -1;
	}
	
	symbols = (Elf_Sym *) ((unsigned long) m->ehdr + symtab->sh_offset);

	sym_cnt = symtab->sh_size / symtab->sh_entsize;


	printf("\n.symtab contains %d entries\n"
	       "============================\n"
	       "\t[NUM]\t[VALUE]\t\t\t[SIZE]\t[TYPE]\t[NAME]\n", sym_cnt);

	
	for (i = 0; i < sym_cnt; i++) {

		printf("\t%d\t%016lx\t%4ld",
		       i,
		       symbols[i].st_value,
		       symbols[i].st_size);

		switch (ELF_ST_TYPE(symbols[i].st_info)) {
		case STT_NOTYPE  :
			printf("\tNOTYPE "); break;  
		case STT_OBJECT  :
			printf("\tOBJECT "); break;
		case STT_FUNC    :
			printf("\tFUNC   "); break;
		case STT_SECTION :
			printf("\tSECTION"); break;
		case STT_FILE    :
			printf("\tFILE   "); break;
		case STT_COMMON  :
			printf("\tCOMMON "); break;
		case STT_TLS     :
			printf("\tTLS    "); break;
		default:
			printf("\tUNKNOWN"); break;
		}

		printf("\t%-10s\n", get_symbol_str(m, i));

	}
	
	return 0;	
}




/**
 * @brief dump the contents of strtab
 */

__attribute__((unused))
static void dump_strtab(const struct elf_module *m)
{
	size_t i;


	if (!m->str)
		return;


	printf("\n.strtab:\n"
	       "============================\n"
	       "\t[OFF]\t[STR]\n");

	while(i < m->sh_size) {
		printf("\t[%d]\t%s\n", i, m->str + i);
		i += strlen(m->str + i) + 1;
	}

	printf("\n\n");
}


/**
 * @brief locate and set the ".shstrtab" section
 *
 * @return 0 if section header string table was not found, 1 otherwise
 *
 */

static int set_shstrtab(struct elf_module *m)
{
	size_t i;

	for (i = 0; i < m->ehdr->e_shnum; i++) {

		if (m->shdr[i].sh_type != SHT_STRTAB)
			continue;

		/* section name index is not within table boundary, skip */
		if (m->shdr[i].sh_size < m->shdr[i].sh_name)
			continue;

		m->sh_str  = (((char *) m->ehdr) + m->shdr[i].sh_offset);
		m->sh_size = m->shdr[i].sh_size;

		/* it's a string section and the size is ok,
		 * now make sure it is the right one */
		if (!strcmp(m->sh_str + m->shdr[i].sh_name, ".shstrtab"))
			return 1;
	}

	m->sh_str = NULL;
	m->sh_size = 0;

	return 0;
}

/**
 * @brief locate and set the ".dynstr" section
 *
 * @return 0 if section header string table was not found, 1 otherwise
 *
 */

static int set_dynstr(struct elf_module *m)
{
	unsigned int idx;


	idx = find_sec(m, ".dynstr");

	if (idx) {
		m->dyn_str  = (((char *) m->ehdr) + m->shdr[idx].sh_offset);
		m->dyn_size = m->shdr[idx].sh_size;
		return 1;
	}
	
	m->dyn_str = NULL;
	m->dyn_size = 0;

	return 0;
}


/**
 * @brief locate and set the ".strtab" section
 *
 * @return 0 if string table was not found, 1 otherwise
 *
 */

static int set_strtab(struct elf_module *m)
{
	unsigned int idx;


	idx = find_sec(m, ".strtab");

	if (idx) {
		m->str      = (((char *) m->ehdr) + m->shdr[idx].sh_offset);
		m->str_size = m->shdr[idx].sh_size; 
		return 1;
	}

	m->str = NULL;
	m->str_size = 0;

	return 0;
}


/**
 * @brief setup the module structure
 *
 * @return -1 on error
 */

static int setup_module(struct elf_module *m)
{
	/* initialise module configuration */
	m->pa       = 0;
	m->va       = 0;
	// XXX
	//m->size     = 0;
	m->align    = sizeof(void *);
	m->dyn      = NULL;
	m->dyn_size = 0;

	m->sec      = NULL;

	/* set section headers */
	if (m->ehdr->e_shoff) {
		m->shdr = (Elf_Shdr *) (((char *) m->ehdr) + m->ehdr->e_shoff);
	} else {
		m->shdr = NULL;
		printf("ERR: no section header found\n");
		return -1;
	}

	/* locate and set section header string table */
	if (!set_shstrtab(m))
		return -1;
	
	/* locate and set dynamic string table */
	if (!set_dynstr(m)) {
		printf("WARN: no dynamic string table found\n");
	}

	/* locate and set string table */
	if (!set_strtab(m))
		return -1;

	/* set up for relocatable object */
	if (m->ehdr->e_type == ET_REL) {
		printf("TODO\n");

		m->align = 0x200000;	/* PC */
#if 0
		int i;
		m->size = 0;
		for (i = 0; i < m->ehdr->e_shnum; i++) {
			if ((m->shdr[i].sh_flags & SHF_ALLOC)) {
				printf("Alloc section: %s, size %ld\n",
				       m->sh_str + m->shdr[i].sh_name,
				       m->shdr[i].sh_size);
				
				m->size += m->shdr[i].sh_size;

				if (m->shdr[idx].sh_addralign > m->align)
		
					m->align = m->shdr[idx].sh_addralign;

			}
		}
#endif
	}

	return 0;
}



static int module_load_mem(struct elf_module *m)
{
	unsigned int idx = 0;
	unsigned long va_load;
	unsigned long pa_load;

	char *src;

	Elf_Shdr *sec;

	struct module_section *s;

	void *mem;


	mem = malloc(m->size);

	if (!mem)
		return -ENOMEM;

	/* exec info */
	m->va = (unsigned long) mem;
	m->pa = (unsigned long) mem;


	printf("\n\nLoading module run-time sections\n");

	va_load = m->va;
	pa_load = m->pa;

	m->num_sec = get_num_alloc_sections(m);
	m->sec = (struct module_section *)
		 malloc(sizeof(struct module_section) * m->num_sec);

	if (!m->sec)
		return -1;


	s = m->sec;

	while (1) {

		idx = find_shdr_alloc(m, idx + 1);
		if (!idx)
			break;

		sec = get_sec(m, idx);


		if (!sec->sh_size)	/* don't need those */
			continue;

		s->size = sec->sh_size;

		src = get_shstrtab_str(m, idx);
		s->name = malloc(strlen(src));

		if (!s->name)
			return -1;

		strcpy(s->name, src);


		if (sec->sh_type & SHT_NOBITS) {
			printf("\tZero segment %10s at %p size %ld\n",
			       s->name, (char *) va_load,
			       sec->sh_size);

			bzero((void *) va_load, s->size);
		} else {
			printf("\tCopy segment %10s from %p to %p size %ld\n",
			       s->name,
			       (char *) m->ehdr + sec->sh_offset,
			       (char *) va_load,
			       sec->sh_size);

			memcpy((void *) va_load,
			       (char *) m->ehdr + sec->sh_offset,
			       sec->sh_size);
		}

		s->addr = va_load;
		va_load = s->addr + s->size;
		
		s++;

		if (s > &m->sec[m->num_sec]) {
			printf("Error out of section memory\n");
			return -1;
		}
	}

	return 0;
}


static int module_relocate(struct elf_module *m)
{
	unsigned int idx = 0;

	size_t i;
	size_t rel_cnt;

	Elf_Shdr *sec;
	


	/* no dynamic linkage, so it's either self-contained or bugged, we'll
	 * assume the former, so cross your fingers and hope for the best
	 */
	if (m->ehdr->e_type != ET_REL)
		if (m->ehdr->e_type != ET_DYN)
			return 0;


	/* we only need RELA type relocations */

	while (1) {

		idx = find_sec_type(m, SHT_RELA, idx + 1);

		if (!idx)
			break;

		sec = get_sec(m, idx);

		printk("\nSection Header info: %ld\n", sec->sh_info);
		
		if (sec) {

			Elf_Rela *relatab;

			rel_cnt = sec->sh_size / sec->sh_entsize;

			printk("Found %d RELA entries\n", rel_cnt);
			/* relocation table in memory */
			relatab = (Elf_Rela *) ((long)m->ehdr + sec->sh_offset);

			for (i = 0; i < rel_cnt; i++) {

				int reladdr;
				unsigned int symsec = ELF_R_SYM(relatab[i].r_info);
				char *symstr = get_symbol_str(m, symsec);
				struct module_section *s;
				struct module_section *text  = find_mod_sec(m, ".text");
				
				printf("OFF: %08lx INF: %8lx ADD: %3ld LNK: %ld SEC: %d NAME: %s\n\n",
				       relatab[i].r_offset,
				       relatab[i].r_info,
				       relatab[i].r_addend,
				       sec->sh_link,
				       symsec,
				       symstr);
			


				if (strlen(symstr)) {
					Elf_Addr sym = (Elf_Addr) lookup_symbol(symstr);

					if (!sym) {

						unsigned long symval;
						printf("\tNot found in library, resolving in module\n");


						if (!(get_symbol_type(m, symstr) & STT_FUNC)) {
							printf("\tERROR, unresolved symbol %s\n", symstr);
							return -1;
						}
						if (!get_symbol_value(m, symstr, &symval)) {
							printf("\tERROR, unresolved symbol %s\n", symstr);
							return -1;
						}

						sym = (text->addr + symval);

					}

					printf("\tSymbol %s at %lx\n", symstr, sym);

					apply_relocate_add(m, &relatab[i], sym);

				} else  { /* no string, symtab entry is probably a section, try to identify it */
				
					char *secstr = get_shstrtab_str(m, symsec);

					s = find_mod_sec(m, secstr);

					if (!s) {
						printf("Error cannot locate section %s for symbol\n", secstr);
							continue;
					}
					/* target address to insert at location */
					reladdr = (long) s->addr;
					printf("\tRelative symbol address: %x, entry at %08lx\n", reladdr, s->addr);
					
					apply_relocate_add(m, &relatab[i], reladdr);
				}

				printf("\n");


			}
			printf("\n");
		}
	}


	return 0;
}





typedef void (*entrypoint_t)(void);
void go(entrypoint_t ep)
{
	ep();
	printk("done\n");
}


int module_load(struct elf_module *m, void *p)
{
	unsigned long symval;
	entrypoint_t ep; 


	/* the ELF binary starts with the ELF header */
	m->ehdr = (Elf_Ehdr *) p;

	printk("Checking ELF header\n");

	if (elf_header_check(m->ehdr))
		return -1;

	printk("Setting up module configuration\n");
	
	if (setup_module(m))
		return -1;


	dump_sections(m);
	
	if (module_load_mem(m))
		return -1;
	
	dump_symtab(m);

	if (module_relocate(m))
		return -1;

#if 1
	//uintptr_t epaddr = (uintptr_t) (m->va);
	if (!get_symbol_value(m, "_module_init", &symval)) {
		printf("module init not found\n");
		return -1;

	}

	ep = (entrypoint_t) (m->va + symval);
	
	printf("Binary entrypoint is %lx; invoking %p\n", m->ehdr->e_entry, ep);

	go(ep);
#endif

	return 0;
}




