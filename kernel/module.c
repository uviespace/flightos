/**
 * @file kernel/module.c
 *
 * @ingroup kernel_module
 * @defgroup kernel_module Loadable Kernel Module Support
 *
 * @brief implements loadable kernel modules
 *
 *
 *
 *
 * TODO 1. module chainloading, reference counting and dependency
 *	   tracking for automatic unloading of unused modules
 *	2. code cleanup
 *	3. ???
 *	4. profit
 */

#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/err.h>
#include <kernel/module.h>
#include <kernel/ksym.h>
#include <kernel/kmem.h>
#include <kernel/kernel.h>



#define MOD "MODULE: "


/* if we need more space, this is how many entries we will add */
#define MODULE_REALLOC 10
/* this is where we keep track of loaded modules */
static struct {
	struct elf_module **m;
	int    sz;
	int    cnt;
} _kmod;


/**
 * @brief find module section by name
 *
 * @param m a struct elf_module
 *
 * @param name the name of the module section
 *
 * @return module section structure pointer or NULL if not found
 */

struct module_section *find_mod_sec(const struct elf_module *m,
				    const char *name)

{
	size_t i;


	if (!name)
		return NULL;

	for (i = 0; i < m->num_sec; i++) {
		if (!strcmp(m->sec[i].name, name))
			return &m->sec[i];
	}


	return NULL;
}


/**
 * @brief setup the module structure
 *
 * @param m a struct elf_module
 *
 * @return -1 on error
 */

static int setup_module(struct elf_module *m)
{
	int i;

	Elf_Shdr *shdr;


	/* initialise module configuration */
	m->pa       = 0;
	m->va       = 0;
	m->size     = 0;
	m->refcnt   = 0;
	m->align    = sizeof(void *);
	m->sec      = NULL;
	m->base     = NULL;
	m->init     = NULL;
	m->exit     = NULL;

	
	/* set up for relocatable object */
	if (m->ehdr->e_type == ET_REL) {
		for (i = 0; i < m->ehdr->e_shnum; i++) {
			shdr = elf_get_sec_by_idx(m->ehdr, i);
			if (!shdr)
				return -1;	

			if (shdr->sh_flags & SHF_ALLOC) {
				pr_debug(MOD "Alloc section: %s, size %ld\n",
					 elf_get_shstrtab_str(m->ehdr, i),
					 shdr->sh_size);

				m->size += shdr->sh_size;

				if (shdr->sh_addralign > m->align) {
					m->align = shdr->sh_addralign;
					pr_debug(MOD "align: %d\n", m->align);
				}

			}
		}
	}

	return 0;
}


/**
 * @brief load the run time sections of a module into memory
 *
 * @param m a struct elf_module
 *
 * @return 0 on success, -ENOMEM on error
 */

static int module_load_mem(struct elf_module *m)
{
	unsigned int idx = 0;
	unsigned long va_load;
	unsigned long pa_load;

	char *src;

	Elf_Shdr *sec;

	struct module_section *s;


	m->base = kmalloc(m->size + m->align);

	if (!m->base)
		goto error;

	/* exec info */
	m->va = (unsigned long) ALIGN_PTR(m->base, m->align);
	m->pa = (unsigned long) ALIGN_PTR(m->base, m->align);

	pr_debug(MOD "\n" MOD "\n"
		MOD "Loading module run-time sections\n");

	va_load = m->va;
	pa_load = m->pa;

	m->num_sec = elf_get_num_alloc_sections(m->ehdr);
	m->sec = (struct module_section *)
		 kcalloc(sizeof(struct module_section), m->num_sec);

	if (!m->sec)
		goto error;


	s = m->sec;

	while (1) {

		idx = elf_find_shdr_alloc_idx(m->ehdr, idx + 1);
		if (!idx)
			break;

		sec = elf_get_sec_by_idx(m->ehdr, idx);


		if (!sec->sh_size)	/* don't need those */
			continue;

		s->size = sec->sh_size;

		src = elf_get_shstrtab_str(m->ehdr, idx);
		if (!src)
			goto error;

		s->name = kmalloc(strlen(src));

		if (!s->name)
			goto error;

		strcpy(s->name, src);


		if (sec->sh_type & SHT_NOBITS) {
			pr_debug(MOD "\tZero segment %10s at %p size %ld\n",
			       s->name, (char *) va_load,
			       sec->sh_size);

			bzero((void *) va_load, s->size);
		} else {
			pr_debug(MOD "\tCopy segment %10s from %p to %p size %ld\n",
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
			pr_debug(MOD "Error out of section memory\n");
			goto error;
		}
	}

	return 0;

error:
	if (m->sec)
		for (idx = 0; idx < m->num_sec; idx++)
			kfree(m->sec[idx].name);

	kfree(m->sec);
	kfree(m->base);

	return -ENOMEM;
}


/**
 * @brief apply relocations to a module
 *
 * @param m a struct elf_module
 *
 * @return 0 on success, otherwise error
 */

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

		idx = elf_find_sec_idx_by_type(m->ehdr, SHT_RELA, idx + 1);

		if (!idx)
			break;

		sec = elf_get_sec_by_idx(m->ehdr, idx);

		pr_debug(MOD "\n"
			MOD "Section Header info: %ld\n", sec->sh_info);

		if (sec) {

			Elf_Rela *relatab;

			rel_cnt = sec->sh_size / sec->sh_entsize;

			pr_debug(MOD "Found %d RELA entries\n", rel_cnt);
			/* relocation table in memory */
			relatab = (Elf_Rela *) ((long)m->ehdr + sec->sh_offset);

			for (i = 0; i < rel_cnt; i++) {

				int reladdr;
				unsigned int symsec = ELF_R_SYM(relatab[i].r_info);
				char *symstr = elf_get_symbol_str(m->ehdr, symsec);
				struct module_section *s;
				struct module_section *text  = find_mod_sec(m, ".text");
				
				pr_debug(MOD "OFF: %08lx INF: %8lx ADD: %3ld LNK: %ld SEC: %d NAME: %s\n",
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
						pr_info(MOD "\tSymbol %s not found in library, trying to resolving in module\n",
							 symstr);


						if (!(elf_get_symbol_type(m->ehdr, symstr) & STT_OBJECT)) {
							pr_debug(MOD "\tERROR, object data resolution not yet implemented, symbol %s may not function as intended. If it is a variable, declare it static as a workaround\n", symstr);
						}

						if (!(elf_get_symbol_type(m->ehdr, symstr) & (STT_FUNC | STT_OBJECT))) {
							pr_err(MOD "\tERROR, unresolved symbol %s\n", symstr);
							return -1;
						}
						if (!elf_get_symbol_value(m->ehdr, symstr, &symval)) {
							pr_err(MOD "\tERROR, unresolved symbol %s\n", symstr);
							return -1;
						}

						sym = (text->addr + symval);

					}

					pr_debug(MOD "\tSymbol %s at %lx\n", symstr, sym);

					apply_relocate_add(m, &relatab[i], sym);

				} else  { /* no string, symtab entry is probably a section, try to identify it */

					char *secstr = elf_get_shstrtab_str(m->ehdr, symsec);

					s = find_mod_sec(m, secstr);

					if (!s) {
						pr_debug(MOD "Error cannot locate section %s for symbol\n", secstr);
							continue;
					}
					/* target address to insert at location */
					reladdr = (long) s->addr;
					pr_debug(MOD "\tRelative symbol address: %x, entry at %08lx\n", reladdr, s->addr);

					apply_relocate_add(m, &relatab[i], reladdr);
				}

				pr_debug(MOD "\n");


			}
			pr_debug(MOD "\n");
		}
	}


	return 0;
}


/**
 * @brief unload a module
 *
 * @param m a struct elf_module
 *
 */

void module_unload(struct elf_module *m)
{
	int i;


	if (m->exit) {
		if (m->exit())
			pr_err(MOD "Module exit call failed.\n");
	}

	if (m->sec) {
		for (i = 0; i < m->num_sec; i++)
			kfree(m->sec[i].name);
	}

	kfree(m->sec);
	kfree(m->base);
}


/**
 * @brief load a module
 *
 * @param m a struct elf_module
 * @param p the address where the module ELF file is located
 *
 * @return 0 on success, -1 on error
 */

int module_load(struct elf_module *m, void *p)
{
	unsigned long symval;


	/* the ELF binary starts with the ELF header */
	m->ehdr = (Elf_Ehdr *) p;

	pr_debug(MOD "Checking ELF header\n");

	if (elf_header_check(m->ehdr))
		goto error;

	pr_debug(MOD "Setting up module configuration\n");

	if (setup_module(m))
		goto error;

	if (module_load_mem(m))
		goto cleanup;

	if (module_relocate(m))
		goto cleanup;

	if (_kmod.cnt == _kmod.sz) {
		_kmod.m = krealloc(_kmod.m, (_kmod.sz + MODULE_REALLOC) *
				   sizeof(struct elf_module **));

		bzero(&_kmod.m[_kmod.sz], sizeof(struct elf_module **) *
		      MODULE_REALLOC);
		_kmod.sz += MODULE_REALLOC;
	}

	_kmod.m[_kmod.cnt++] = m;

	if (elf_get_symbol_value(m->ehdr, "_module_init", &symval))
		m->init = (void *) (m->va + symval);
	else
		pr_warn(MOD "_module_init() not found\n");


	if (elf_get_symbol_value(m->ehdr, "_module_exit", &symval))
		m->exit = (void *) (m->va + symval);
	else
		pr_warn(MOD "_module_exit() not found\n");


	pr_debug(MOD "Binary entrypoint is %lx; invoking _module_init() at %p\n",
		m->ehdr->e_entry, m->init);

	if (m->init) {
		if (m->init()) {
			pr_err(MOD "Module initialisation failed.\n");
			goto cleanup;
		}
	}


	return 0;

cleanup:
	module_unload(m);
error:
	return -1;
}


/**
 * @brief list all loaded modules
 */

void modules_list_loaded(void)
{
	struct elf_module **m;


	m = _kmod.m;

	while((*m)) {
		printk(MOD "Contents of module %p loaded at base %p\n"
		       MOD "\n",
		       (*m), (*m)->base);

		elf_dump_sections((*m)->ehdr);
		elf_dump_symtab((*m)->ehdr);

		m++;
	}
}
