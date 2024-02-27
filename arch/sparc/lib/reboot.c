/**
 * @file  arch/sparc/kernel/reboot.c
 *
 * architecture-specific implementations required by kernel/reboot.h
 */

#include <asm/leon.h>
#include <asm/ttable.h>

void machine_halt(void)
{
	__asm__ __volatile__ ("ta 2\n\t":::);
	while(1);
}

void die(void)
{
	put_psr(get_psr() & ~PSR_ET);
	__asm__ __volatile__ ("ta 80\n\t":::);
}
