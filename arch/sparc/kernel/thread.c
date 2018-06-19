/**
 * @file arch/sparc/kernel/thread.c
 * 
 * @ingroup sparc
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
 *
 * @brief implements the architecture specific thread component
 *
 */


#include <asm/thread.h>

#include <asm/irqflags.h>
#include <asm/leon.h>

#include <kernel/export.h>
#include <kernel/kthread.h>

#define MSG "SPARC THREAD: "


extern struct thread_info *current_set[];




/**
 * @brief this is a wrapper that actually executes the thread function
 */

static void th_starter(void)
{
	struct task_struct *task = current_set[0]->task;

	task->thread_fn(task->data);

	printk("thread: %p returned\n", task->thread_fn);
	task->state = TASK_DEAD;

	schedule();

	pr_crit(MSG "should never have reached %s:%d", __func__, __LINE__);
	BUG();
}




/**
 * @brief initialise a task structure
 */

void arch_init_task(struct task_struct *task,
		    int (*thread_fn)(void *data),
		    void *data)
{

#define STACKFRAME_SZ	96
#define PTREG_SZ	80
#define PSR_CWP     0x0000001f
	task->thread_info.ksp = (unsigned long) task->stack_top - (STACKFRAME_SZ + PTREG_SZ);
	task->thread_info.kpc = (unsigned long) th_starter - 8;
	task->thread_info.kpsr = get_psr();
	task->thread_info.kwim = 1 << (((get_psr() & PSR_CWP) + 1) % 8);
	task->thread_info.task = task;

	task->thread_fn = thread_fn;
	task->data      = data;
}
EXPORT_SYMBOL(arch_init_task);



/**
 * @brief promote the currently executed path to a task
 * @note we use this to move our main thread to the task list
 */

void arch_promote_to_task(struct task_struct *task)
{
#define PSR_CWP     0x0000001f

	task->thread_info.ksp = (unsigned long) leon_get_fp();
	task->thread_info.kpc = (unsigned long) __builtin_return_address(1) - 8;
	task->thread_info.kpsr = get_psr();
	task->thread_info.kwim = 1 << (((get_psr() & PSR_CWP) + 1) % 8);
	task->thread_info.task = task;

	task->thread_fn = NULL;
	task->data      = NULL;


	printk(MSG "kernel stack %x\n", leon_get_fp());

	printk(MSG "is next at %p stack %p\n", &task->thread_info, task->stack);


	/* and set the new thread as current */
	__asm__ __volatile__("mov	%0, %%g6\n\t"
			     :: "r"(&(task->thread_info)) : "memory");


}
EXPORT_SYMBOL(arch_promote_to_task);
