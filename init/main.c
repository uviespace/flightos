/**
 * @file init/main.c
 */

#include <generated/autoconf.h>

#include <kernel/init.h>

#include <kernel/module.h> /* module_load */

#include <kernel/ksym.h> /* lookup_symbol */

#include <kernel/kernel.h>
#include <kernel/printk.h>
#include <kernel/kmem.h>
#include <kernel/sbrk.h>


void module_image_load_embedded(void);
void *module_lookup_embedded(char *mod_name);
void *module_lookup_symbol_embedded(char *sym_name);



static void kernel_init(void)
{
	setup_arch();
}


#ifdef CONFIG_TARGET_COMPILER_BOOT_CODE

int main(void)
{
	kernel_init();

#if 0
	{
		struct elf_module m;
		void *addr;

		printk("%s at %p\n", "printk", lookup_symbol("printk"));

		module_image_load_embedded();

		/*  load -binary kernel/test.ko 0xA0000000 */
		/* module_load(&m, (char *) 0xA0000000); */


		addr = kmalloc(4096);
		//addr = module_lookup_embedded("testmodule.ko");
		addr = module_lookup_symbol_embedded("somefunction");

		if (addr)
			module_load(&m, addr);
	}
#endif
	return 0;
}

#endif /* CONFIG_TARGET_COMPILER_BOOT_CODE */
