/**
 * @file   arch/sparc/kernel/write_buffer_error.c
 *
 * @ingroup sparc
 *
 * @brief handle unaligned memory access
 *
 * @note this is a form of FDIR, as all unaligned access on this
 *	 platform should be considered an implementation error
 */

#include <asm/leon.h>

void kernel_write_buffer_error(void)
{
	leon3_flush_dcache();
}
