/**
 * @file   arch/sparc/kernel/stacktrace.c
 */

#include <stdlib.h>

#include <compiler.h>
#include <asm/leon.h>
#include <stacktrace.h>
#include <kernel/printk.h>



/**
 * @brief validates the stack pointer address
 */
static int stack_valid(uint32_t sp)
{
	if (sp & (STACK_ALIGN - 1) || !sp)
		return 0;

	return 1;
}


/**
 * @brief performs a stack trace
 *
 * @param trace  a struct stack_trace
 * @param sp     a stack/frame pointer
 * @param pc     a program counter
 *
 * @note When being called from a trap, the pc in %o7 is NOT the return program
 *       counter of the trapped function, so a stack/frame pointer by itself
 *       is not enough to provide a proper trace, hence the pc argument
 */

void save_stack_trace(struct stack_trace *trace, uint32_t sp, uint32_t pc)
{
	struct sparc_stackf *sf;
	struct pt_regs *regs;


	if (!stack_valid(sp))
		return;

	/* flush register windows to memory*/
	leon_reg_win_flush();

	do {
		if (!stack_valid(sp))
			break;


		sf   = (struct sparc_stackf *) sp;
		regs = (struct pt_regs *) (sf + 1);

		trace->frames[trace->nr_entries] = sf;
		trace->regs[trace->nr_entries]   = regs;

		trace->nr_entries++;

		pc = sf->callers_pc;
		sp = (uint32_t) sf->fp;

	} while (trace->nr_entries < trace->max_entries);
}



#if defined(USE_STACK_TRACE_TRAP)

/**
 * @brief a generic shutdown function
 * TODO: replace with whatever we need in the end
 */

void die(void)
{
	BUG();
}


/**
 * @brief executes a stack trace
 *
 * @param fp a frame pointer
 * @param pc a program counter
 *
 * @note this is called by the trap handler
 */

void trace(uint32_t fp, uint32_t pc)
{
	uint32_t entries[30];

	struct stack_trace x;


	x.max_entries = 30;
	x.nr_entries  = 0;
	x.entries     = entries;

	save_stack_trace(&x, fp, pc);
}
#endif
