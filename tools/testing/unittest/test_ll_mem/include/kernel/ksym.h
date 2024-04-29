#ifndef _KERNEL_KSYM_H_
#define _KERNEL_KSYM_H_



struct ksym {
	char *name;
	void *addr;
};

extern struct ksym __ksyms[];

void *lookup_symbol(const char *name);

#endif /* _KERNEL_KSYM_H_ */
