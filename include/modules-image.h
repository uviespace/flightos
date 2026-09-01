/**
 * @file include/modules-image.h
 */

#ifndef _MODULES_IMAGE_H_
#define _MODULES_IMAGE_H_

/**
 * @brief load all modules embedded in the module image
 */
void module_image_load_embedded(void);

/**
 * @brief look up an embedded module by name
 * @param mod_name: module name to find
 * @return pointer to the module, or NULL if not found
 */
void *module_lookup_embedded(char *mod_name);

/**
 * @brief look up an exported symbol within embedded modules
 * @param sym_name: symbol name to find
 * @return pointer to the symbol, or NULL if not found
 */
void *module_lookup_symbol_embedded(char *sym_name);

/**
 * @brief read the raw data of an embedded module
 * @param mod_name: module name to read
 * @return pointer to the module data, or NULL if not found
 */
void *module_read_embedded(char *mod_name);

#if defined(CONFIG_XENTIUM)
/**
 * @brief load all embedded Xentium kernels
 */
void module_load_xen_kernels(void);
#endif /* CONFIG_XENTIUM */

#endif /* _MODULES_IMAGE_H_ */
