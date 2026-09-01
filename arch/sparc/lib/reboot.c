/**
 * @file  arch/sparc/kernel/reboot.c
 *
 * architecture-specific implementations required by kernel/reboot.h
 */
#include <kernel/reboot.h>

#include <asm/leon.h>
#include <asm/ttable.h>

/**
 * @brief halt the machine
 *
 * @param reason an 8-bit reason code passed to the reset trap (ta 0x2)
 *
 * @note this function never returns
 */

void machine_halt(uint8_t reason)
{
        long _r = (long)reason;


        __asm__ __volatile__(
                             "mov       %0, %%g7    \n\t"
                             "ta        0x2         \n\t"
                             :
                             : "r"  (_r)
                             : "memory", "%g7");

	__asm__ __volatile__ ("ta 2\n\t":::);
	while(1);
}

/**
 * @brief disable traps and enter an infinite error loop trap (ta 80)
 *
 * @note this function never returns
 */

void die(void)
{
	put_psr(get_psr() & ~PSR_ET);
	__asm__ __volatile__ ("ta 80\n\t":::);
}
