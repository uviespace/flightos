/**
 * @file    include/asm-generic/thread.h
 * @ingroup schedthread
 * @ingroup threadsys
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
 * @brief Architecture-neutral hooks for initializing and promoting task cores.
 *
 */

#ifndef _ASM_GENERIC_THREAD_H_
#define _ASM_GENERIC_THREAD_H_

#include <asm/thread.h>

struct task_core;
struct task_struct;

/**
 * @brief architecture-specific initialization of a task core
 * @param core: the task_core to initialize
 * @param task: the task_struct to initialize
 * @param thread_fn: thread entry point function
 * @param data: argument passed to thread_fn
 */
void arch_init_task(struct task_core *core,
		    struct task_struct *task,
		    int (*thread_fn)(void *data),
		    void *data);

/**
 * @brief promote a task_core to a fully runnable task
 * @param core: the task_core to promote
 * @param task: the task_struct being promoted
 */
void arch_promote_to_task(struct task_core *core, struct task_struct *task);

#endif /* _ASM_GENERIC_THREAD_H_ */
