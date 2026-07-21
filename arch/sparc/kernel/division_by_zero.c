/**
 * @file   arch/sparc/kernel/hw_div0.c
 *
 * @ingroup sparc
 *
 * @brief handle hardware division by zero
 */

#include <kernel/reboot.h>

void hw_div0(void)
{
	machine_halt(REBOOT_DIV0);
}
