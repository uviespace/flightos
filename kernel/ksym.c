/**
 * @file kernel/ksym.c
 *
 * @brief kernel symbol table
 *
 * Defines the global kernel symbol table (filled at compile time by the
 * link/ksym generation step) and provides a lookup routine to resolve a
 * symbol name to its runtime address.
 */

#include <string.h>

#include <kernel/ksym.h>



/* global kernel symbol table, filled at compile time */
struct ksym __ksyms[] __attribute__((weak)) = {{NULL, NULL}};


/**
 * @brief look up a kernel symbol by name
 *
 * @param name the symbol name to search for
 *
 * @return pointer to the symbol address, or NULL if not found
 */

void *lookup_symbol(const char *name)
{
	struct ksym *s = &__ksyms[0];

	while (s->name) {
		if(!strcmp(s->name, name))
			return s->addr;
		s++;
	}

	return NULL;
}
