/**
 * @file include/kernel/ksym.h
 *
 * @brief kernel symbol table interface
 *
 * Declares the global kernel symbol table and the lookup routine used to
 * resolve a symbol name to its runtime address.
 */

#ifndef _KERNEL_KSYM_H_
#define _KERNEL_KSYM_H_



/**
 * @brief kernel symbol entry mapping a name to an address
 */
struct ksym {
	char *name;	/*!< symbol name string */
	void *addr;	/*!< symbol address in memory */
};

/** @brief external reference to the kernel symbol table */
extern struct ksym __ksyms[];

/**
 * @brief look up a kernel symbol by name
 * @param name: the symbol name to search for
 * @return pointer to the symbol address, or NULL if not found
 */
void *lookup_symbol(const char *name);

#endif /* _KERNEL_KSYM_H_ */
