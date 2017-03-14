/**
 * @file kernel/xentium.c
 *
 */


#include <kernel/printk.h>
#include <kernel/err.h>
#include <kernel/xentium.h>
#include <kernel/kmem.h>
#include <kernel/kernel.h>
#include <asm-generic/swab.h>
#include <kernel/string.h>
#include <elf.h>










#define MSG "XEN: "



/* if we need more space, this is how many entries we will add */
#define KERNEL_REALLOC 10
/* this is where we keep track of loaded modules */
static struct {
	struct xen_kernel **x;
	int    sz;
	int    cnt;
} _xen;




/**
 * @brief setup the module structure
 *
 * @return -1 on error
 */

static int xentium_setup_kernel(struct xen_kernel *m)
{
	int i;

	Elf_Shdr *shdr;


	/* initialise module configuration */
	m->size     = 0;
	m->refcnt   = 0;
	m->align    = sizeof(void *);
	m->sec      = NULL;
	m->ep       = m->ehdr->e_entry;



	for (i = 0; i < m->ehdr->e_shnum; i++) {
		shdr = elf_get_sec_by_idx(m->ehdr, i);
		if (!shdr)
			return -1;	

		if (shdr->sh_flags & SHF_ALLOC) {
			printk(MSG "found alloc section: %s, size %ld\n",
			       elf_get_shstrtab_str(m->ehdr, i),
			       shdr->sh_size);

			m->size += shdr->sh_size;

			if (shdr->sh_addralign > m->align) {
				m->align = shdr->sh_addralign;
				printk(MSG "align: %d\n", m->align);
			}

		}
	}

	return 0;
}



/**
 * @brief check if this is a valid Xentium ELF binary
 * @note this requires elf32-big instead of elf32-xentium in linker script
 *	 target
 */

int xentium_elf_header_check(Elf_Ehdr *ehdr)
{
	if (ehdr->e_ident[EI_DATA] != ELFDATA2MSB) {
		pr_debug(MSG "swapping kernel ELF header endianess");
		elf_hdr_endianess_swap(ehdr);
	}

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


	if (!(ehdr->e_ident[EI_VERSION] == EV_CURRENT))
		return -1;

#if 0
	/* the Xentium toolchain cannot produce files with relocations,
	 * which is very sad...
	 */
	if (!(ehdr->e_type == ET_REL)) {
		return -1;
	}
	
	if(!ehdr->e_machine != XXX)
		return -1;
#endif

	if (!(ehdr->e_version == EV_CURRENT))
		return -1;

	return 0;
}


/**
 * @brief load a xentium kernel
 *
 * @note since xentium programs are not relocatable, this just dumps
 * them wherever they say they want to be. The user is responsible for the
 * proper memory configuration and offsets, I really can't do much about that at
 * the moment, because I simply do not have the time to just work with the
 * object files and do all relocations by hand, because that would also require
 * me to take care of the Xentium's runtime wrapper code, which again would take
 * more time that I can currently spare, but this would be the preferred
 * solution.
 */

static int xentium_load_kernel(struct xen_kernel *x)
{
	unsigned int idx = 0;

	char *src;

	Elf_Shdr *sec;

	struct xen_module_section *s;

	uint32_t *p;

	int i;


	printk(MSG "\n" MSG "\n"
	       MSG "Loading kernel run-time sections\n");

	x->num_sec = elf_get_num_alloc_sections(x->ehdr);
	x->sec = (struct xen_module_section *)
		kcalloc(sizeof(struct xen_module_section), x->num_sec);

	if (!x->sec)
		goto error;

	s = x->sec;

	while (1) {

		idx = elf_find_shdr_alloc_idx(x->ehdr, idx + 1);


		if (!idx)
			break;

		sec = elf_get_sec_by_idx(x->ehdr, idx);

		if (!sec->sh_size)	/* don't need those */
			continue;

		s->size = sec->sh_size;

		src = elf_get_shstrtab_str(x->ehdr, idx);
		s->name = kmalloc(strlen(src));

		if (!s->name)
			goto error;

		strcpy(s->name, src);

		if (sec->sh_type & SHT_NOBITS) {
			printk(MSG "\tZero segment %10s at %p size %ld\n",
			       s->name, (char *) sec->sh_addr,
			       sec->sh_size);

			bzero((void *) sec->sh_addr, s->size);
		} else {
			printk(MSG "\tcopy segment %10s from %p to %p size %ld\n",
			       s->name,
			       (char *) x->ehdr + sec->sh_offset,
			       (char *) sec->sh_addr,
			       sec->sh_size);

			memcpy((void *) sec->sh_addr,
			       (char *) x->ehdr + sec->sh_offset,
			       sec->sh_size);

			printk(MSG "\tadjust byte ordering\n");

			p = (uint32_t *) sec->sh_addr;
			for (i = 0; i < sec->sh_size / sizeof(uint32_t); i++)
				p[i] = swab32(p[i]);



		}

		s->addr = sec->sh_addr;

		s++;

		if (s > &x->sec[x->num_sec]) {
			printk(MSG "Error out of section memory\n");
			goto error;
		}
	}

	return 0;

error:
	if (x->sec)
		for (idx = 0; idx < x->num_sec; idx++)
			kfree(x->sec[idx].name);

	kfree(x->sec);

	return -ENOMEM;
}



#include <asm-generic/swab.h>
int xentium_kernel_load(struct xen_kernel *x, void *p)
{
	unsigned long symval;


	/* the ELF binary starts with the ELF header */
	x->ehdr = (Elf_Ehdr *) p;

	printk(MSG "Checking ELF header\n");



	if (xentium_elf_header_check(x->ehdr))
		goto error;

	printk(MSG "Setting up module configuration\n");

	if (xentium_setup_kernel(x))
		goto error;

	if (xentium_load_kernel(x))
		goto cleanup;



	if (_xen.cnt == _xen.sz) {
		_xen.x = krealloc(_xen.x, (_xen.sz + KERNEL_REALLOC) *
				   sizeof(struct xen_kernel **));

		bzero(&_xen.x[_xen.sz], sizeof(struct xen_kernel **) *
		      KERNEL_REALLOC);
		_xen.sz += KERNEL_REALLOC;
	}

	_xen.x[_xen.cnt++] = x;

	return 0;
cleanup:
	printk("cleanup\n");
#if 0
	xentium_kernel_unload(m);
#endif
error:
	printk("error\n");
	return -1;
}

