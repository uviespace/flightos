/**
 * linker references to embedded modules.image
 */

#include <kernel/printk.h>
#include <kernel/ar.h>
#include <kernel/kmem.h>
#include <kernel/string.h>
#include <kernel/xentium.h>

#define MSG "MODIMG: "


extern unsigned char _binary_modules_image_start __attribute__((weak));
extern unsigned char _binary_modules_image_end __attribute__((weak));
extern unsigned char _binary_modules_image_size __attribute__((weak));

struct archive mod_ar;


void module_image_load_embedded(void)
{
	ar_load(&_binary_modules_image_start,
		(unsigned int)&_binary_modules_image_size, &mod_ar);

#if 0
	ar_print_files(&mod_ar);
	ar_print_symbols(&mod_ar);
#endif
}


void *module_lookup_symbol_embedded(char *sym_name)
{
	return ar_find_symbol(&mod_ar, sym_name);
}

void *module_lookup_embedded(char *mod_name)
{
	return ar_find_file(&mod_ar, mod_name);
}

void *module_read_embedded(char *mod_name)
{
	unsigned int fsize;

	void *ptr;

	fsize = ar_read_file(&mod_ar, mod_name, NULL);

	ptr = kmalloc(fsize);
	if (!ptr)
		return NULL;

	if (!ar_read_file(&mod_ar, mod_name, ptr)) {
		kfree(ptr);
		return NULL;
	}

	return ptr;
}


/**
 * @brief try to load all xentium kernels found in the embedded image
 *
 * @note assumes all files with suffix ".xen" are a kernel
 */

void module_load_xen_kernels(void)
{
	char *list;
	char *fname;
	void *file;

	struct xen_kernel x;


	list = ar_get_file_list(&mod_ar);

	if (!list)
		return;


	while (1) {
		fname = strsep(&list, " ");
		if (!fname)
			break;

		if (!strstr(fname, ".xen"))
			continue;

		pr_info(MSG "Loading Xentium kernel %s\n", fname);

		file = module_read_embedded(fname);
		if (!file)
			pr_err(MSG "Failed to read file %s\n", fname);

		if (!xentium_kernel_load(&x, file))
			pr_err(MSG "Error loading Xentium kernel %s\n", fname);

		kfree(file);

	}

	kfree(list);
}
