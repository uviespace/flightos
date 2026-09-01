/**
 * @file   arch/sparc/include/traps.h
 * @ingroup interrupts
 * @ingroup traps
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 * @date   February, 2016
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
 */


#ifndef _ARCH_SPARC_TRAPS_H_
#define _ARCH_SPARC_TRAPS_H_

#include <kernel/types.h>

/**
 * @brief installs a custom trap handler
 *
 * @param trap a trap entry number
 * @param handler a function to call when the trap is triggerd
 */
void trap_handler_install(uint32_t trap, void (*handler)(void));

/**
 * @brief trap handler for a data access (MMU or EDAC) exception
 *
 * Saves the trap window and dispatches to data_access_exception().
 */
void data_access_exception_trap(void);

/**
 * @brief ignore a data access exception
 *
 * Skips the faulting instruction to continue execution.
 */
void data_access_exception_trap_ignore(void);

/**
 * @brief trap handler for a floating point exception
 *
 * Saves the trap window and dispatches to fpe_trap().
 */
void floating_point_exception_trap(void);

#endif /* _ARCH_SPARC_TRAPS_H_ */
