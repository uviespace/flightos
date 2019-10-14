/**
 * @file    spinlock.h
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 * @date    December, 2013
 * @brief   spin locking
 *
 * @copyright Armin Luntzer (armin.luntzer@univie.ac.at)
 * @copyright David S. Miller (davem@caip.rutgers.edu), 1997  (some parts)
 *
 * @copyright
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 *
 * XXX Since RDPSR and WRPSR are in principle interruptible, none of these
 *     functions should be used to actually modify the PSR/PIL
 *     rather, we'll need a software trap that raises the PIL for use,
 *     i.e. call it up via a ticc instruction, so that ET=0 when raising the
 *     PIL. This should however only be done in supervisor mode for obvious
 *     reasons, otherwise the trap must have no effect.
 */

#ifndef _ARCH_SPARC_ASM_SPINLOCK_H_
#define _ARCH_SPARC_ASM_SPINLOCK_H_

#include <compiler.h>
#include <kernel/types.h>

#define __SPINLOCK
struct spinlock {
	uint8_t	 lock;
	uint32_t lock_recursion;
};


/* the sparc PIL field	*/
#define PSR_PIL 0x00000f00



/**
 * @brief save and disable the processor interrupt level state
 * @return PSR
 * @warning make sure to call a save/restore pair from within the same stack frame
 */

#define __spin_lock_save_irq __spin_lock_save_irq
__attribute__((unused))
static uint32_t spin_lock_save_irq(void)
{
	uint32_t psr;
	uint32_t tmp;


	__asm__ __volatile__(
			"rd     %%psr, %0        \n\t"
			"or        %0, %2, %1    \n\t"
			"wr        %1,  0, %%psr \n\t"
			"nop                     \n\t"
			"nop                     \n\t"
			"nop                     \n\t"
			: "=&r" (psr), "=r" (tmp)
			: "i"   (PSR_PIL)
			: "memory");
	return psr;
}


/**
 * @brief Restore the processor interrupt level state
 */

#define __spin_lock_restore_irq __spin_lock_restore_irq
__attribute__((unused))
static void spin_lock_restore_irq(uint32_t psr)
{
	uint32_t tmp;

	__asm__ __volatile__(
			"rd     %%psr, %0        \n\t"
			"and       %2, %1, %2    \n\t"
			"andn      %0, %1, %0    \n\t"
			"wr        %0, %2, %%psr \n\t"
			"nop                     \n\t"
			"nop                     \n\t"
			"nop                     \n\t"
			: "=&r" (tmp)
			: "i"   (PSR_PIL), "r" (psr)
			: "memory");
}


/**
 * @brief	MPPB LEON-side spin lock
 * @param 	lock a struct spinlock
 *
 * @warning	will silently fail AND deadlock everytime YOU are stupid
 * @note	it is, however, save to use with interrupts (sort of)
 */

#define __spin_lock __spin_lock
__attribute__((unused))
static void spin_lock(struct spinlock *lock)
{
	uint32_t psr_flags;

	if (unlikely(lock->lock_recursion))
		return;

	psr_flags = spin_lock_save_irq();

	lock->lock_recursion = 1;

	__asm__ __volatile__(
			"1:                      \n\t"
			"ldstub [%0], %%g2       \n\t"
			"andcc  %%g2, %%g2, %%g2 \n\t"
			"bnz,a    1b             \n\t"
			" nop		         \n\n"
			:
			: "r" (&lock->lock)
			: "g2", "memory", "cc");

	lock->lock_recursion = 0;

	spin_lock_restore_irq(psr_flags);
}


/**
 * @brief	spin lock which does not care about interrupts
 * @param 	lock a struct spinlock
 */

__attribute__((unused))
static void spin_lock_raw(struct spinlock *lock)
{
#if 0
	if (unlikely(lock->lock_recursion))
		return;

	lock->lock_recursion = 1;
#endif
	__asm__ __volatile__(
			"1:                      \n\t"
			"ldstub [%0], %%g2       \n\t"
			"andcc  %%g2, %%g2, %%g2 \n\t"
			"bnz,a    1b             \n\t"
			" nop		         \n\n"
			:
			: "r" (&lock->lock)
			: "g2", "memory", "cc");
#if 0
	lock->lock_recursion = 0;
#endif
}

/**
 * @brief   lock check
 * @returns success or failure
 */

#define __spin_is_locked __spin_is_locked
__attribute__((unused))
static int spin_is_locked(struct spinlock *lock)
{
	return (lock->lock != 0);
}


/**
 * @brief spins forever until lock opens
 */

#define __spin_unlock_wait __spin_unlock_wait
__attribute__((unused))
static void spin_unlock_wait(struct spinlock *lock)
{
	barrier();

	while(spin_is_locked(lock));
}

/**
 * @brief	spin lock
 * @param 	lock a struct spinlock
 * @return	success or failure
 */

#define __spin_try_lock __spin_try_lock
__attribute__((unused))
static int spin_try_lock(struct spinlock *lock)
{
	uint32_t retval;

	__asm__ __volatile__("ldstub [%1], %0" : "=r" (retval) : "r"  (&lock->lock) : "memory");

	return (retval == 0);
}

/**
 * @brief	side spin-unlock
 * @param 	lock a struct spinlock
 */

#define __spin_unlock __spin_unlock
__attribute__((unused))
static void spin_unlock(struct spinlock *lock)
{
	__asm__ __volatile__("swap [%0], %%g0       \n\t" : : "r" (&lock->lock) : "memory");
}



#endif
