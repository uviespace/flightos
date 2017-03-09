#ifndef _KERNEL_MODULE_H_
#define _KERNEL_MODULE_H_

#include <kernel/init.h>
#include <kernel/elf.h>



#define module_init(initfunc)   \
        int _module_init(void) __attribute__((alias(#initfunc)));

#define module_exit(exitfunc)   \
        int _module_exit(void) __attribute__((alias(#exitfunc)));



struct module_section {
	char *name;
	unsigned long addr;
	size_t size;
};


struct elf_module {

	unsigned long pa;
	unsigned long va;

	void *base;

	int (*init)(void);
	int (*exit)(void);

	int refcnt;

	unsigned int align;

	Elf_Ehdr *ehdr;		/* coincides with start of module image */
	Elf_Shdr *shdr;
	Elf_Dyn  *dyn;

	size_t size;
	size_t dyn_size;
	size_t sh_size;
	size_t str_size;

	char  *dyn_str;		/* dynamic symbols string tab */
	char  *sh_str;		/* section header string tab */
	char  *str;		/* stringtab */

	struct module_section *sec;
	size_t num_sec;
};



/* implemented in architecture code */
int apply_relocate_add(struct elf_module *m, Elf_Rela *rel, Elf_Addr sym);



struct module_section *find_mod_sec(const struct elf_module *m,
				    const char *name);

int module_load(struct elf_module *m, void *p);
void modules_list_loaded(void);

#endif /* _KERNEL_MODULE_H_ */
