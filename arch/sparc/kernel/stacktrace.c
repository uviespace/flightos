/**
 * @file   arch/sparc/kernel/stacktrace.c
 *
 * @ingroup sparc
 *
 * @brief stack tracking for the SPARC target
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

	while (trace->nr_entries < trace->max_entries) {

		sf   = (struct sparc_stackf *) sp;
		regs = (struct pt_regs *) (sf + 1);

		trace->frames[trace->nr_entries] = sf;
		trace->regs[trace->nr_entries]   = regs;

		trace->nr_entries++;

		pc = sf->callers_pc;
		sp = (uint32_t) sf->fp;

		if (!stack_valid(sp))
			break;
	}
}



#if defined(USE_STACK_TRACE_TRAP)

/**
 * @brief a generic shutdown function
 * TODO: replace with whatever we need in the end
 */

void die(void)
{
	machine_halt();
}


/**
 * @brief executes a stack trace
 *
 * @param fp a frame pointer
 * @param pc a program counter
 *
 * @note this is called by the trap handler
 *
 */

void trace(uint32_t fp, uint32_t pc)
{
#define MAX_ENTRIES 30
	struct stack_trace x;
        struct sparc_stackf *frames[MAX_ENTRIES];
        struct pt_regs      *regs[MAX_ENTRIES];

	x.max_entries = MAX_ENTRIES;
	x.nr_entries  = 0;
	x.frames      = frames;
	x.regs        = regs;

	save_stack_trace(&x, fp, pc);
}
#endif
