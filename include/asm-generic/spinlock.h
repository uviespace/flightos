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
static unsigned long spin_lock_save_irq(void)
{
	return 0;
}
#endif


#ifndef __spin_lock_restore_irq
#define __spin_lock_restore_irq __spin_lock_restore_irq
static void spin_lock_restore_irq(__attribute__((unused)) uint32_t psr)
{
}
#endif


#ifndef __spin_lock
#define __spin_lock __spin_lock
static void spin_lock(__attribute__((unused)) struct spinlock *lock)
{
}
#endif


#ifndef __spin_is_locked
#define __spin_is_locked __spin_is_locked
static int spin_is_locked(__attribute__((unused)) struct spinlock *lock)
{
	return 0;
}
#endif


#ifndef __spin_unlock_wait
#define __spin_unlock_wait __spin_unlock_wait
static void spin_unlock_wait(__attribute__((unused)) struct spinlock *lock)
{
}
#endif

#ifndef __spin_try_lock
#define __spin_try_lock __spin_try_lock
static int spin_try_lock(__attribute__((unused)) struct spinlock *lock)
{
	return 0;
}
#endif

#ifndef __spin_unlock
#define __spin_unlock __spin_unlock
static void spin_unlock(__attribute__((unused)) struct spinlock *lock)
{
}
#endif

#define /* _ASM_GENERIC_SPINLOCK_H_ */
