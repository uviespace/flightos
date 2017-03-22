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



#define MSG "MAIN: "

void module_image_load_embedded(void);
void *module_lookup_embedded(char *mod_name);
void *module_lookup_symbol_embedded(char *sym_name);
void *module_read_embedded(char *mod_name);
void module_load_xen_kernels(void);

static void kernel_init(void)
{
	setup_arch();
#ifdef CONFIG_SYSCTL
	sysctl_init();
#endif
}


#include <data_proc_net.h>
#include <kernel/xentium.h>

int xen_op_output(unsigned long op_code, struct proc_task *t)
{
	ssize_t i;
	ssize_t n;

	unsigned int *p = NULL;


	n = pt_get_nmemb(t);
	printk("XEN OUT: op code %d, %d items\n", op_code, n);

	if (!n)
		goto exit;


	p = (unsigned int *) pt_get_data(t);
	if (!p)
		goto exit;



	for (i = 0; i < n; i++) {
		printk("\t%d\n", p[i]);
	}

exit:
	kfree(p);	/* clean up our data buffer */

	pt_destroy(t);

	return PN_TASK_SUCCESS;
}

void xen_new_input_task(size_t n)
{
	struct proc_task *t;

	static int seq;

	int i;
	unsigned int *data;



	data = kzalloc(sizeof(unsigned int) * n);

	for (i = 0; i < n; i++)
		data[i] = i;


	t = pt_create(data, n, 3, 0, seq++);

	BUG_ON(!t);

	BUG_ON(pt_add_step(t, 0xdeadbeef, NULL));
	BUG_ON(pt_add_step(t, 0xdeadbee0, NULL));

	xentium_input_task(t);
}






#ifdef CONFIG_TARGET_COMPILER_BOOT_CODE

int main(void)
{
	void *addr;
	struct elf_module m;


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



#if 0
	addr = module_read_embedded("noc_dma.ko");

	pr_debug(MSG "noc_dma module address is %p\n", addr);

	if (addr)
		module_load(&m, addr);

#endif
#if 0
	modules_list_loaded();
#endif








#if 1
	/* load all available Xentium kernels from the embedded modules image */
	module_load_xen_kernels();


	xentium_config_output_node(xen_op_output);


	xen_new_input_task(3);
	xen_new_input_task(3);
	xen_new_input_task(3);

	xentium_schedule_next();
	xentium_schedule_next();
	xentium_schedule_next();
	xentium_schedule_next();
	xentium_schedule_next();


#endif

	return 0;
}

#endif /* CONFIG_TARGET_COMPILER_BOOT_CODE */
