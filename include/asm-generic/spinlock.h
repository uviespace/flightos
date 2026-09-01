/**
 * @file    include/asm-generic/spinlock.h
 *
 * @copyright GPLv2
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @note this is nothing but a placeholder...
 */

#ifndef _ASM_GENERIC_SPINLOCK_H_
#define _ASM_GENERIC_SPINLOCK_H_

#include <asm/spinlock.h>


#ifndef __SPINLOCK
#define __SPINLOCK
struct spinlock {
}
#endif

#ifndef __spin_lock_save_irq
#define __spin_lock_save_irq __spin_lock_save_irq

/**
 * @brief lock a spinlock while saving and disabling IRQs
 * @return saved IRQ flags (may be 0 in generic placeholder)
 */
static unsigned long spin_lock_save_irq(void)
{
	return 0;
}
#endif


#ifndef __spin_lock_restore_irq
#define __spin_lock_restore_irq __spin_lock_restore_irq
/**
 * @brief unlock a spinlock while restoring IRQ flags
 * @param psr: IRQ flags to restore
 */
static void spin_lock_restore_irq(__attribute__((unused)) uint32_t psr)
{
}
#endif


#ifndef __spin_lock
#define __spin_lock __spin_lock
/**
 * @brief acquire a spinlock (generic no-op placeholder)
 * @param lock: the spinlock to acquire
 */
static void spin_lock(__attribute__((unused)) struct spinlock *lock)
{
}
#endif


#ifndef __spin_is_locked
#define __spin_is_locked __spin_is_locked
/**
 * @brief check whether a spinlock is held (generic placeholder)
 * @param lock: the spinlock to check
 * @return non-zero if locked, 0 otherwise
 */
static int spin_is_locked(__attribute__((unused)) struct spinlock *lock)
{
	return 0;
}
#endif


#ifndef __spin_unlock_wait
#define __spin_unlock_wait __spin_unlock_wait
/**
 * @brief wait until a spinlock is unlocked (generic no-op placeholder)
 * @param lock: the spinlock to wait on
 */
static void spin_unlock_wait(__attribute__((unused)) struct spinlock *lock)
{
}
#endif

#ifndef __spin_try_lock
#define __spin_try_lock __spin_try_lock
/**
 * @brief try to acquire a spinlock without blocking (generic placeholder)
 * @param lock: the spinlock to try
 * @return non-zero on success (lock acquired), 0 on failure
 */
static int spin_try_lock(__attribute__((unused)) struct spinlock *lock)
{
	return 0;
}
#endif

#ifndef __spin_unlock
#define __spin_unlock __spin_unlock
/**
 * @brief release a spinlock (generic no-op placeholder)
 * @param lock: the spinlock to release
 */
static void spin_unlock(__attribute__((unused)) struct spinlock *lock)
{
}
#endif

#endif /* _ASM_GENERIC_SPINLOCK_H_ */
