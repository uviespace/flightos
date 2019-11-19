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
 * @brief find module section by index
 *
 * @param m a struct elf_module
 *
 * @param idx the index of the module section
 *
 * @return module section structure pointer or NULL if not found
 */

struct module_section *find_mod_idx(const struct elf_module *m,
				    size_t idx)

{
	if (idx >= m->num_sec)
		return NULL;

	return &m->sec[idx];
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


#if 0
		if (!sec->sh_size)	/* don't need those */
			continue;
#else
		if (!(sec->sh_flags & SHF_ALLOC))
			continue;
#endif

		s->size = sec->sh_size;

		src = elf_get_shstrtab_str(m->ehdr, idx);
		if (!src)
			goto error;

		s->name = kmalloc(strlen(src));

		if (!s->name)
			goto error;

		strcpy(s->name, src);
		
		pr_err(MOD "section %s index %d max %d\n", s->name, s, m->num_sec);


		if (sec->sh_type & SHT_NOBITS) {
			pr_info(MOD "\tZero segment %10s at %p size %ld\n",
			       s->name, (char *) va_load,
			       sec->sh_size);

			bzero((void *) va_load, s->size);
		} else {
			pr_info(MOD "\tCopy segment %10s from %p to %p size %ld\n",
			       s->name,
			       (char *) m->ehdr + sec->sh_offset,
			       (char *) va_load,
			       sec->sh_size);

			if (sec->sh_size)
				memcpy((void *) va_load,
				       (char *) m->ehdr + sec->sh_offset,
				       sec->sh_size);
		}

		s->addr = va_load;
		va_load = s->addr + s->size;

		s++;

		if (s > &m->sec[m->num_sec]) {
			pr_err(MOD "Error out of section memory\n");
			goto error;
		}
	}

#if 0
	if (elf_get_common_size(m->ehdr)) {

		m->num_sec++;
		m->sec = (struct module_section *)
			krealloc(m->sec, sizeof(struct module_section) * m->num_sec);

		if (!m->sec)
			goto error;

		s = &m->sec[m->num_sec - 1];

		s->size = elf_get_common_size(m->ehdr);
		s->addr = (unsigned long) kzalloc(s->size);
		if (!s->addr)
			goto error;

		pr_info(MOD "\tcreating .alloc section of %d bytes at %x\n", s->size, s->addr);

		s->name = kmalloc(strlen(".alloc"));
		if (!s->name)
			goto error;

		strcpy(s->name, ".alloc");

	}
#else
	/* yes, yes, fixup like everything else here */
	if (elf_get_common_objects(m->ehdr, NULL)) {
		size_t cnt = elf_get_common_objects(m->ehdr, NULL);
		char **obj;

		m->sec = (struct module_section *)
			krealloc(m->sec, sizeof(struct module_section) * (m->num_sec + cnt));

		if (!m->sec)
			goto error;


		/* point to last */
		s = &m->sec[m->num_sec];
		/* and update */
		m->num_sec += cnt;

		obj = kmalloc(cnt * sizeof(char **));
		if (!obj)
			goto error;
		/* load names */
		elf_get_common_objects(m->ehdr, obj);


		/* Allocate one section entry for each object, this is not
		 * that efficient, but what the heck. Better not to have
		 * uninitialised non-static globals at all!
		 */
		for (idx = 0; idx < cnt; idx++) {
			s->size = elf_get_symbol_size(m->ehdr, obj[idx]);
			s->addr = (unsigned long) kzalloc(s->size);
			if (!s->addr)
				goto error;
			pr_debug(MOD "\tcreating \"%s\" section of %d bytes at %x\n", obj[idx], s->size, s->addr);
			s->name = kmalloc(strlen(obj[idx]));
			if (!s->name)
				goto error;
			strcpy(s->name, obj[idx]);
			s++;
		}

		kfree(obj);
	}
#endif



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

		char *rel_sec;

		idx = elf_find_sec_idx_by_type(m->ehdr, SHT_RELA, idx + 1);

		if (!idx)
			break;

		sec = elf_get_sec_by_idx(m->ehdr, idx);

		pr_info(MOD "\n"
			MOD "Section Header info: %ld %s\n", sec->sh_info, elf_get_shstrtab_str(m->ehdr, idx));

		if (!strcmp(elf_get_shstrtab_str(m->ehdr, idx), ".rela.text"))
			rel_sec = ".text";
		else if (!strcmp(elf_get_shstrtab_str(m->ehdr, idx), ".rela.data"))
			rel_sec = ".data";
		else
			BUG();


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
				struct module_section *text  = find_mod_sec(m, ".text"); /* ?? rel_sec ?? */

				pr_debug(MOD "OFF: %08lx INF: %8lx ADD: %3ld LNK: %ld SEC: %d NAME: %s\n",
				       relatab[i].r_offset,
				       relatab[i].r_info,
				       relatab[i].r_addend,
				       sec->sh_link,
				       symsec,
				       symstr);



				if (strlen(symstr)) {
						unsigned long symval;
						Elf_Addr sym = (Elf_Addr) lookup_symbol(symstr);

						if (!sym) {
							pr_debug(MOD "\tSymbol %s not found in library, trying to resolving in module\n",
								 symstr);

							if ((elf_get_symbol_type(m->ehdr, symstr) & STT_OBJECT)) {
								char *secstr;



								s = find_mod_sec(m, elf_get_shstrtab_str(m->ehdr, elf_get_symbol_shndx(m->ehdr, symstr)));

								switch (elf_get_symbol_shndx(m->ehdr, symstr)) {
								case SHN_UNDEF:
									pr_debug(MOD "\tundefined section index\n");
									break;
								case SHN_ABS:
									pr_debug(MOD "\tabsolute value section index\n");
								case SHN_COMMON:
							     		pr_debug(MOD "\t %s common symbol index (must allocate) add: %d\n", symstr,
										relatab[i].r_addend);
									s = find_mod_sec(m, symstr);
									if (!s) {
										pr_err("no suitable section found");
										return -1;
									}
									break;
								default:
									break;

								}


								if (!s) {
									pr_debug(MOD "Error cannot locate section %s for symbol\n", secstr);
									continue;
								}
								secstr = s->name;
								/* target address to insert at location */
								reladdr = (long) s->addr;
							pr_debug(MOD "\tRelative symbol address: %x, entry at %08lx, section %s\n", reladdr, s->addr, secstr);

							/* needed ?? */
							//elf_get_symbol_value(m->ehdr, symstr, (unsigned long *) &relatab[i].r_addend);
								apply_relocate_add(m, &relatab[i], reladdr, rel_sec);
								continue;

							}

#if 0
						if (!(elf_get_symbol_type(m->ehdr, symstr) & (STT_FUNC | STT_OBJECT))) {
							pr_err(MOD "\tERROR, unresolved symbol %s\n", symstr);
							return -1;
						}
#endif
						if (!elf_get_symbol_value(m->ehdr, symstr, &symval)) {
							pr_err(MOD "\tERROR, unresolved symbol %s\n", symstr);
							return -1;
						}

						sym = (text->addr + symval);

					}

					pr_debug(MOD "\tSymbol %s at %lx val %lx sec %d\n", symstr, sym, symval, symsec);

					apply_relocate_add(m, &relatab[i], sym, rel_sec);

				} else  { /* no string, symtab entry is probably a section, try to identify it */

					char *secstr = elf_get_shstrtab_str(m->ehdr, symsec);

					s = find_mod_idx(m, symsec-1);

					if (!s) {
						pr_debug(MOD "Error cannot locate section %s for symbol\n", secstr);
							continue;
					}
					secstr = s->name;
					/* target address to insert at location */
					reladdr = (long) s->addr;
					pr_debug(MOD "\tRelative symbol address: %x, entry at %08lx, section %s\n", reladdr, s->addr, secstr);

					apply_relocate_add(m, &relatab[i], reladdr, rel_sec);
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
