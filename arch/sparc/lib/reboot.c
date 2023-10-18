/**
 * @file  arch/sparc/kernel/reboot.c
 *
 * architecture-specific implementations required by kernel/reboot.h
 */


void machine_halt(void)
{
	__asm__ __volatile__ ("ta 2\n\t":::);
}
