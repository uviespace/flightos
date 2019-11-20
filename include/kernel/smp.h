/**
 * @file    include/kernel/smp.h
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
* @brief kernel-level SMP support functions
 *
 */


#ifndef _KERNEL_SMP_H_
#define _KERNEL_SMP_H_


extern void smp_init(void);

extern int smp_cpu_id(void);

extern void smp_send_reschedule(int cpu);



#endif /* _KERNEL_SMP_H_ */
