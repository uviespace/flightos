/**
 * @file   arch/sparc/kernel/traps.c
 * @ingroup traps
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 * @author Linus Torvalds et al.
 * @date   September, 2015
 *
 * @copyright GPLv2
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @defgroup traps Trap Table access
 * @brief Implements functionality to access or modify SPARC v8 MVT trap
 *	  table entries
 *
 *
 *
 * ## Overview
 *
 * This module implements functionality to access or modify SPARC v8 MVT trap
 * table entries.
 *
 * ## Mode of Operation
 *
 * None
 *
 * ## Error Handling
 *
 * None
 *
 * ## Notes
 *
 * - functionality will be added as needed
 *
 *
 */

#include <traps.h>
#include <asm/io.h>
#include <compiler.h>


/**
 * @brief installs a trap handler that executes a long jump
 *
 * @param trap a trap entry number
 * @param handler a function to call when the trap is triggerd
 *
 * @note I'm not sure why this must be nested in another function call. If it
 *       isn't, the trap entry seemingly does not update properly. It might have
 *       to do with the rotation of the register window or some caching
 *       mechanism I'm not aware of at this time. The writes to the table are
 *       uncached, so the data cache is likely not the issue, and flusing it
 *       won't do anything either. When not nested in a wrapper, it however
 *       appears to work as expected if a function that in turn calls a
 *       function is executed after the trap entry is written - but it does not
 *       if the same function is called after this one has returned. O.o
 *
 * Installs a custom handler into a specified trap table entry. Note that it is
 * not possible to install just any function, since traps are disabled when the
 * call is executed and any further trap (such as window over/underflow) will
 * force the processor to jump to trap 0 (i.e. reset). See lib/asm/trace_trap.S
 * for a working example.
 *
 *
 */

static void trap_longjump_install(uint32_t trap, void (*handler)(void))
{
	uint32_t *trap_base;
	uint32_t tbr;
	uint32_t h;


	h = (unsigned int) handler;

	/* extract the trap table address from %tbr (skips lower bits 0-11) */
	__asm__ __volatile__("rd %%tbr, %0" : "=r" (tbr));

	/* calculate offset to trap, each entry is 4 machine words long */
	trap_base = (uint32_t *)((tbr & ~0xfff) + (trap << 4));

	/* set up the trap entry:
	 * 0x29000000 sethi %hi(handler), %l4
	 * 0x81c52000 jmpl  %l4 + %lo(handler), %g0
	 * 0x01000000 rd    %psr, %l0
	 * 0x01000000 nop
	 */
	iowrite32be(((h >> 10) & 0x3fffff) | 0x29000000, trap_base + 0);
	iowrite32be((h        &    0x3ff) | 0x81c52000, trap_base + 1);
	iowrite32be(0xa1480000,                         trap_base + 2);
	iowrite32be(0x01000000,                         trap_base + 3);

	barrier();
}

/**
 * @brief installs a custom trap handler
 *
 * @param trap a trap entry number
 * @param handler a function to call when the trap is triggerd
 *
 * Installs a custom handler into a specified trap table entry. Note that it is
 * not possible to install just any function, since traps are disabled when the
 * call is executed and any further trap (such as window over/underflow) will
 * force the processor to jump to trap 0 (i.e. reset). See lib/asm/trace_trap.S
 * for a working example.
 */

void trap_handler_install(uint32_t trap, void (*handler)(void))
{
	trap_longjump_install(trap, handler);
}
