/**
 * @file arch/sparc/kernel/setup.c
 */

#include <string.h> /* memset() */

#include <init.h>
#include <mm.h>
#include <compiler.h>


/**
 * @brief configure available memory banks
 *
 * TODO the memory layout should either be presented in a separate
 *	board configuration file or, preferably, be derived from an AMBA
 *	bus scan.
 */

static void mem_init(void)
{
	memset(&sp_banks, 0x0, ARRAY_SIZE(sp_banks));

	sp_banks[0].base_addr = 0x40000000;
	sp_banks[0].num_bytes = 0x00800000;
#if 0
	sp_banks[1].base_addr = 0x60000000;
	sp_banks[1].num_bytes = 0x04000000;
#endif
}


/**
 * @brief architecture setup entry point
 */

void setup_arch(void)
{
	mem_init();
	paging_init();
}
