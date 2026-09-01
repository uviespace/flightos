/**
 * @file   arch/sparc/kernel/hw_div0.c
 *
 * @ingroup sparc
 *
 * @brief handle hardware division by zero
 */

#include <kernel/reboot.h>

/**
 * @brief handle a hardware division by zero trap
 *
 * @note halts the machine with REBOOT_DIV0
 */

void hw_div0(void)
{
	machine_halt(REBOOT_DIV0);
}
