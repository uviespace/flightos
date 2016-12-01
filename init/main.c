/**
 * @file init/main.c
 */

#include <generated/autoconf.h>

#include <kernel/init.h>


static void kernel_init(void)
{
	setup_arch();
}


#ifdef CONFIG_TARGET_COMPILER_BOOT_CODE

int main(void)
{
	kernel_init();

	return 0;
}

#endif /* CONFIG_TARGET_COMPILER_BOOT_CODE */
