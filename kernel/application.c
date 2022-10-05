/**
 * @file kernel/application.c
 *
 * @ingroup kernel_application
 * @defgroup kernel_application Loadable Kernel Module Support
 *
 * @brief implements loadable kernel applications
 *
 *
 */

#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/err.h>
#include <kernel/ksym.h>
#include <kernel/kmem.h>
#include <kernel/kernel.h>
#include <kernel/kthread.h>

#include <kernel/module.h>


#define MSG "APP: "


/* if we need more space, this is how many entries we will add */
#define APP_REALLOC 10
/* this is where we keep track of loaded applications */
static struct {
	struct elf_binary **m;
	int    sz;
	int    cnt;
} _kmod;




/**
 * @brief load the run time sections of a application into memory
 *
 * @param m a struct elf_binary
 *
 * @return 0 on success, -ENOMEM on error
 */

static int application_load_mem(struct elf_binary *m)
{
	unsigned int idx = 0;
	unsigned long va_load;
	unsigned long pa_load;

	char *src;

	Elf_Shdr *sec;

	struct elf_section *s;


	if (m->ehdr->e_type == ET_REL) {
		/* this is for relocatible binaries, we can put them anywhere */
		m->base = kmalloc(m->size + m->align);

		if (!m->base)
			goto error;

		/* exec info */
		/* XXX we only do 1:1 mapping or PA only at the moment */
		m->va = (unsigned long) ALIGN_PTR(m->base, m->align);
		m->pa = m->va;


	} else if (m->ehdr->e_type == ET_EXEC) {
		/* non-relocatible binaries, they have to be moved to the proper address */
		pr_err(MSG "FIXME: for non-relocatible binaries, we require the memory\n"
			   "address to either bei virtual (requires MMU) or physically reserved\n"
			   "as a separate partition. There is currently no such mechanism implement.\n"
			   "We hence just assume that the physical region in question is 1) not used\n"
			   "and 2) actually exists.\n");

		/* exec info */
		/* XXX we only do 1:1 mapping or PA only at the moment */
		m->va = m->ehdr->e_entry;
		m->pa = m->va;

	} else {
		pr_err(MSG "unsupported elf type: %d\n", m->ehdr->e_type);
		goto error;
	}


	printk(MSG "va: %08x pa: %08x, base: %08x, align: %08x size: %08x\n", m->va, m->pa, m->base, m->align, m->size);

	pr_debug(MSG "\n"
		 MSG "\n"
		 MSG "Loading application run-time sections\n");

	va_load = m->va;
	pa_load = m->pa;

	m->num_sec = elf_get_num_alloc_sections(m->ehdr);
	m->sec = (struct elf_section *)
		 kcalloc(sizeof(struct elf_section), m->num_sec);

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

		pr_info(MSG "section %s index %p max %d\n", s->name, s, m->num_sec);

		if (sec->sh_type & SHT_NOBITS) {
			pr_info(MSG "\tZero segment %10s at %p size %ld\n",
			       s->name, (char *) sec->sh_addr,
			       sec->sh_size);

			bzero((void *) sec->sh_addr, s->size);
		} else {
#if 1 /* FIXME this should move the segment to the proper memory address, we currently fake it below for testing */
			pr_info(MSG "\tCopy segment %10s from %p to %p size %ld\n",
			       s->name,
			       (char *) m->ehdr + sec->sh_offset,
			       (char *) sec->sh_addr,
			       sec->sh_size);

			if (sec->sh_size)
				memmove((void *) sec->sh_addr,
				       (char *) m->ehdr + sec->sh_offset,
				       sec->sh_size);
#endif
		}

		s->addr = va_load;
		va_load = s->addr + s->size;

		s++;

		if (s > &m->sec[m->num_sec]) {
			pr_err(MSG "Error out of section memory\n");
			goto error;
		}
	}

#if 0
	if (elf_get_common_size(m->ehdr)) {

		m->num_sec++;
		m->sec = (struct elf_section *)
			krealloc(m->sec, sizeof(struct elf_section) * m->num_sec);

		if (!m->sec)
			goto error;

		s = &m->sec[m->num_sec - 1];

		s->size = elf_get_common_size(m->ehdr);
		s->addr = (unsigned long) kzalloc(s->size);
		if (!s->addr)
			goto error;

		pr_info(MSG "\tcreating .alloc section of %d bytes at %x\n", s->size, s->addr);

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

		m->sec = (struct elf_section *)
			krealloc(m->sec, sizeof(struct elf_section) * (m->num_sec + cnt));

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
			pr_info(MSG "\tcreating \"%s\" section of %d bytes at %x\n", obj[idx], s->size, s->addr);
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
 * @brief apply relocations to a application
 *
 * @param m a struct elf_binary
 *
 * @return 0 on success, otherwise error
 */

static int application_relocate(struct elf_binary *m)
{
	unsigned int idx = 0;

	size_t i;
	size_t rel_cnt;

	Elf_Shdr *sec;

#if 0
	/* no dynamic linkage, so it's either self-contained or bugged, we'll
	 * assume the former, so cross your fingers and hope for the best
	 */
	if (m->ehdr->e_type != ET_REL)
		if (m->ehdr->e_type != ET_DYN)
			return 0;
#endif
	printk("e_type: %d\n", m->ehdr->e_type);
	/* we only need RELA type relocations */

	while (1) {

		char *rel_sec;

		idx = elf_find_sec_idx_by_type(m->ehdr, SHT_RELA, idx + 1);

		if (!idx)
			break;

		sec = elf_get_sec_by_idx(m->ehdr, idx);


		pr_info(MSG "\n"
			MSG "Section Header info: %ld %s\n", sec->sh_info, elf_get_shstrtab_str(m->ehdr, idx));

		if (!strcmp(elf_get_shstrtab_str(m->ehdr, idx), ".rela.text"))
			rel_sec = ".text";
		else if (!strcmp(elf_get_shstrtab_str(m->ehdr, idx), ".rela.data"))
			rel_sec = ".data";
		else if (!strcmp(elf_get_shstrtab_str(m->ehdr, idx), ".rela.rodata"))
			rel_sec = ".rodata";
		else if (!strcmp(elf_get_shstrtab_str(m->ehdr, idx), ".rela.text.startup"))
			rel_sec = ".text.startup";
		else if (!strcmp(elf_get_shstrtab_str(m->ehdr, idx), ".rela.eh_frame"))
			rel_sec = ".eh_frame";
		else {
			printk(MSG "unknown section %s\n", elf_get_shstrtab_str(m->ehdr, idx));
			BUG();
		}


		if (sec) {

			Elf_Rela *relatab;

			rel_cnt = sec->sh_size / sec->sh_entsize;

			pr_debug(MSG "Found %d RELA entries\n", rel_cnt);
			/* relocation table in memory */
			relatab = (Elf_Rela *) ((long)m->ehdr + sec->sh_offset);

			for (i = 0; i < rel_cnt; i++) {

				int reladdr;
				unsigned int symsec = ELF_R_SYM(relatab[i].r_info);
				char *symstr = elf_get_symbol_str(m->ehdr, symsec);
				struct elf_section *s;
				struct elf_section *text  = find_elf_sec(m, ".text"); /* ?? rel_sec ?? */

				pr_debug(MSG "OFF: %08lx INF: %8lx ADD: %3ld LNK: %ld SEC: %d NAME: %s\n",
				       relatab[i].r_offset,
				       relatab[i].r_info,
				       relatab[i].r_addend,
				       sec->sh_link,
				       symsec,
				       symstr);


				if (symstr && strlen(symstr)) {

						unsigned long symval;
						Elf_Addr sym = (Elf_Addr) lookup_symbol(symstr);

						if (!sym) {
							pr_debug(MSG "\tSymbol %s not found in library, trying to resolving in application\n",
								 symstr);

							if ((elf_get_symbol_type(m->ehdr, symstr) & STT_OBJECT)) {
								char *secstr = NULL;



								s = find_elf_sec(m, elf_get_shstrtab_str(m->ehdr, elf_get_symbol_shndx(m->ehdr, symstr)));

								switch (elf_get_symbol_shndx(m->ehdr, symstr)) {
								case SHN_UNDEF:
									pr_debug(MSG "\tundefined section index\n");
									break;
								case SHN_ABS:
									pr_debug(MSG "\tabsolute value section index\n");
								case SHN_COMMON:
							     		pr_debug(MSG "\t %s common symbol index (must allocate) add: %d\n", symstr,
										relatab[i].r_addend);
									s = find_elf_sec(m, symstr);
									if (!s) {
										pr_err("no suitable section found");
										return -1;
									}
									break;
								default:
									break;

								}


								if (!s) {
									pr_debug(MSG "Error cannot locate section for symbol %s\n", symstr);
									continue;
								}
								secstr = s->name;
								/* target address to insert at location */
								reladdr = (long) s->addr;
							pr_debug(MSG "\tRelative symbol address: %x, entry at %08lx, section %s\n", reladdr, s->addr, secstr);

							/* needed ?? */
							//elf_get_symbol_value(m->ehdr, symstr, (unsigned long *) &relatab[i].r_addend);
								apply_relocate_add(m, &relatab[i], reladdr, rel_sec);
								continue;

							}

#if 0
						if (!(elf_get_symbol_type(m->ehdr, symstr) & (STT_FUNC | STT_OBJECT))) {
							pr_err(MSG "\tERROR, unresolved symbol %s\n", symstr);
							return -1;
						}
#endif
						if (!elf_get_symbol_value(m->ehdr, symstr, &symval)) {
							pr_err(MSG "\tERROR, unresolved symbol %s\n", symstr);
							return -1;
						}

						sym = (text->addr + symval);

					}

					pr_debug(MSG "\tSymbol %s at %lx val %lx sec %d\n", symstr, sym, symval, symsec);

					apply_relocate_add(m, &relatab[i], sym, rel_sec);

				} else  { /* no string, symtab entry is probably a section, try to identify it */

					char *secstr = elf_get_shstrtab_str(m->ehdr, symsec);

					s = find_elf_idx(m, symsec-1);

					if (!s) {
						pr_debug(MSG "Error cannot locate section %s for symbol\n", secstr);
							continue;
					}
					secstr = s->name;
					/* target address to insert at location */
					reladdr = (long) s->addr;
					pr_debug(MSG "\tRelative symbol address: %x, entry at %08lx, section %s\n", reladdr, s->addr, secstr);

					apply_relocate_add(m, &relatab[i], reladdr, rel_sec);
				}

				pr_debug(MSG "\n");


			}
			pr_debug(MSG "\n");
		}
	}


	return 0;
}



/**
 * @brief unload an application
 *
 * @param app a struct elf_binary
 *
 */

static void application_unload(struct elf_binary *app)
{
	int i;

	if (app->sec) {
		for (i = 0; i < app->num_sec; i++)
			kfree(app->sec[i].name);
	}

	kfree(app->sec);
	kfree(app->base);
	kfree(app);
}


static int application_run_process(void *data)
{
	int ret;

	struct elf_binary *app;


	app = (struct elf_binary *) data;

	ret = app->init();

	pr_notice(MSG "application exited with code %d\n", ret);

	application_unload(app);

	return 0;
}



static int application_create_process(struct elf_binary *app,
				      const char *namefmt,
				      int cpu)
{
	int ret;

	struct task_struct *t;


	t = kthread_create(application_run_process, app, cpu, namefmt);

	ret = kthread_wake_up(t);
	if (ret < 0)
		pr_err(MSG "Unable to create process, kthread_wake_up failed with %d\n", ret);

	return ret;
}


/**
 * @brief load a application
 *
 * @param p the address where the application ELF file is located
 * @param namefmt a application name format string
 * @param cpu the cpu affinity, any number or KTHREAD_CPU_AFFINITY_NONE
 *
 * @return 0 on success, -1 on error
 */

int application_load(void *p, const char *namefmt, int cpu)
{
	struct elf_binary *app;

	unsigned long symval;



	app = (struct elf_binary *) kmalloc(sizeof(struct elf_binary));
	if (!app)
		goto error;

	/* the ELF binary starts with the ELF header */
	app->ehdr = (Elf_Ehdr *) p;

	pr_debug(MSG "Checking ELF header\n");

	if (elf_header_check(app->ehdr))
		goto error;

	pr_debug(MSG "Setting up application configuration\n");

	if (setup_elf_binary(app))
		goto error;

	if (application_load_mem(app))
		goto cleanup;

	if (application_relocate(app))
		goto cleanup;

	/* resize array -> XXX function */
	if (_kmod.cnt == _kmod.sz) {
		_kmod.m = krealloc(_kmod.m, (_kmod.sz + APP_REALLOC) *
				   sizeof(struct elf_binary **));

		bzero(&_kmod.m[_kmod.sz], sizeof(struct elf_binary **) *
		      APP_REALLOC);
		_kmod.sz += APP_REALLOC;
	}

	_kmod.m[_kmod.cnt++] = app;


	if (elf_get_symbol_value(app->ehdr, "_start", &symval)) {
		app->init = (void *) (symval);

		pr_info(MSG "Binary entrypoint is %lx; invoking _application_init() at %p\n",
		app->ehdr->e_entry, app->init);
	}
	else {
		pr_warn(MSG "symbol _start not found\n");
		goto cleanup;
	}

	if (application_create_process(app, namefmt, cpu)) {
		pr_err(MSG "Failed to create process\n");
		goto cleanup;
	}


	return 0;

cleanup:
	application_unload(app);
error:
	return -1;
}


/**
 * @brief list all loaded applications
 */

void applications_list_loaded(void)
{
	struct elf_binary **m;


	if (!_kmod.m) {
		printk(MSG "no applications were loaded\n");
		return;
	}

	m = _kmod.m;

	while((*m)) {
		printk(MSG "Contents of application %p loaded at base %p\n"
		       MSG "\n",
		       (*m), (*m)->base);

		elf_dump_sections((*m)->ehdr);
		elf_dump_symtab((*m)->ehdr);

		m++;
	}
}
