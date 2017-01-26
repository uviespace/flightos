/**
 * @file init/main.c
 */

#include <generated/autoconf.h>

#include <kernel/init.h>

#include <kernel/module.h> /* module_load */

#include <kernel/ksym.h> /* lookup_symbol */

#include <kernel/printk.h>


int module_load(struct elf_module *m, void *p);

static void kernel_init(void)
{
	setup_arch();
}


#ifdef CONFIG_TARGET_COMPILER_BOOT_CODE

int main(void)
{

	struct elf_module m;

	kernel_init();

	printk("%s at %p\n", "printk", lookup_symbol("printk"));
	printk("%s at %p\n", "printf", lookup_symbol("printf"));


	module_load(&m, (char *) 0xA0000000);
	/*  load -binary kernel/test.ko 0xA0000000 */

	return 0;
}

#endif /* CONFIG_TARGET_COMPILER_BOOT_CODE */
