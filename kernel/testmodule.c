/**
 * @file kernel/test.c
 */

#include <page.h>

#include <kernel/printk.h>


void _module_init(void);

void _module_init(void)
{
	int i;

	printk("moopmoop\n");
	
	for (i = 0; i < 10; i++)
		printk("the cow says: %d\n", i);

	printk("allocating: %p\n", page_alloc());

}
