/**
 * linker references to embedded modules.image
 */

#include <kernel/printk.h>
#include <kernel/ar.h>
#include <kernel/kmem.h>

extern unsigned char _binary_modules_image_start __attribute__((weak));
extern unsigned char _binary_modules_image_end __attribute__((weak));
extern unsigned char _binary_modules_image_size __attribute__((weak));

struct archive mod_ar;


void module_image_load_embedded(void)
{
	ar_load(&_binary_modules_image_start,
		(unsigned int)&_binary_modules_image_size, &mod_ar);

#if 0
	ar_list_files(&mod_ar);
	ar_list_symbols(&mod_ar);
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
