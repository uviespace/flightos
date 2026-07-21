/**
 * @file  arch/sparc/kernel/reboot.c
 *
 * architecture-specific implementations required by kernel/reboot.h
 */
#include <kernel/reboot.h>

#include <asm/leon.h>
#include <asm/ttable.h>

void machine_halt(uint8_t reason)
{
        long _o0 = (long)reason;


        __asm__ __volatile__(
                             "mov       %0, %%o0    \n\t"
                             "ta        0x2         \n\t"
                             :
                             : "r"  (_o0)
                             : "memory");

	__asm__ __volatile__ ("ta 2\n\t":::);
	while(1);
}

void die(void)
{
	put_psr(get_psr() & ~PSR_ET);
	__asm__ __volatile__ ("ta 80\n\t":::);
}
