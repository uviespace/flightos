/**
 * @file include/modules-image.h
 */

#ifndef _MODULES_IMAGE_H_
#define _MODULES_IMAGE_H_

void module_image_load_embedded(void);
void *module_lookup_embedded(char *mod_name);
void *module_lookup_symbol_embedded(char *sym_name);
void *module_read_embedded(char *mod_name);

#if defined(CONFIG_XENTIUM)
void module_load_xen_kernels(void);
#endif /* CONFIG_XENTIUM */

#endif /* _MODULES_IMAGE_H_ */
