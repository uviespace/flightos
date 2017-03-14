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
#include <kernel/sysctl.h>

#include <kernel/xentium.h>

#define MSG "MAIN: "

void module_image_load_embedded(void);
void *module_lookup_embedded(char *mod_name);
void *module_lookup_symbol_embedded(char *sym_name);
void *module_read_embedded(char *mod_name);

static void kernel_init(void)
{
	setup_arch();
#ifdef CONFIG_SYSCTL
	sysctl_init();
#endif
}


#ifdef CONFIG_TARGET_COMPILER_BOOT_CODE

int main(void)
{
	void *addr;
	struct elf_module m;
	struct xen_kernel x;

	kernel_init();

	/* load the embedded AR image */
	module_image_load_embedded();

#if 0
	/* look up a kernel symbol */
	printk("%s at %p\n", "printk", lookup_symbol("printk"));

	/* look up a file in the embedded image */
	printk("%s at %p\n", "noc_dma.ko",
	       module_lookup_embedded("noc_dma.ko"));
#endif
	/* to load an arbitrary image, you may upload it via grmon, e.g.
	 *	load -binary kernel/test.ko 0xA0000000
	 * then load it:
	 *	module_load(&m, (char *) 0xA0000000);
	 */

	/* lookup the module containing <symbol> */
	/* addr = module_lookup_symbol_embedded("somefunction"); */
	/* XXX the image is not necessary aligned properly, so we can't access
	 * it directly, until we have a MNA trap */
#if 1
	addr = module_read_embedded("testkernel.ko");

	printk(MSG "testkernel module address is %p\n", addr);

	if (addr)
		xentium_kernel_load(&x, addr);
#endif

#if 0
	addr = module_read_embedded("noc_dma.ko");

	pr_debug(MSG "noc_dma module address is %p\n", addr);

	if (addr)
		module_load(&m, addr);

#endif
#if 0
	modules_list_loaded();
#endif
	return 0;
}

#endif /* CONFIG_TARGET_COMPILER_BOOT_CODE */
