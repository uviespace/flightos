#include <string.h>

#include <kernel/ksym.h>



/* global kernel symbol table, filled at compile time */
struct ksym __ksyms[] __attribute__((weak)) = {{NULL, NULL}};


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
