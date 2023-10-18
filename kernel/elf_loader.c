/**
 * @file kernel/elf_loader.c
 *
 * @ingroup elf_loader
 * @defgroup elf_loader Loadable Kernel Module and Application Support
 *
 */


#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/elf_loader.h>


#define MSG "ELF LOAD: "

/**
 * @brief find elf section by name
 *
 * @param m a struct elf_binary
 *
 * @param name the name of the elf section
 *
 * @return elf section structure pointer or NULL if not found
 */

struct elf_section *find_elf_sec(const struct elf_binary *m, const char *name)

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
 * @brief find elf section by index
 *
 * @param m a struct elf_binary
 *
 * @param idx the index of the elf section
 *
 * @return elf section structure pointer or NULL if not found
 */

struct elf_section *find_elf_idx(const struct elf_binary *m, size_t idx)

{
	if (idx >= m->num_sec)
		return NULL;

	return &m->sec[idx];
}


/**
 * @brief setup the binary structure
 *
 * @param m a struct elf_binary
 *
 * @return -1 on error
 */

int setup_elf_binary(struct elf_binary *m)
{
	int i;

	Elf_Shdr *shdr;


	/* initialise */
	m->pa       = 0;
	m->va       = 0;
	m->size     = 0;
	m->refcnt   = 0;
	m->align    = sizeof(void *);
	m->sec      = NULL;
	m->base     = NULL;
	m->init     = NULL;
	m->exit     = NULL;



	for (i = 0; i < m->ehdr->e_shnum; i++) {

		shdr = elf_get_sec_by_idx(m->ehdr, i);
		if (!shdr)
			return -1;

		if (shdr->sh_flags & SHF_ALLOC) {
			pr_debug(MSG "Alloc section: %s, size %ld\n",
				 elf_get_shstrtab_str(m->ehdr, i),
				 shdr->sh_size);

			m->size += shdr->sh_size;

			if (shdr->sh_addralign > m->align) {
				m->align = shdr->sh_addralign;
				pr_debug(MSG "align: %d\n", m->align);
			}

		}
	}

	return 0;
}
