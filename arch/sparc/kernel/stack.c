/**
 * @file arch/sparc/kernel/stack.c
 */


#include <stacktrace.h>
#include <asm/leon.h>

#include <string.h> /* memcpy() */
#include <stdio.h>
#include <errno.h>


/**
 * @brief migrate a stack
 *
 * @param sp the (bottom) stack pointer of the old stack
 * @param stack_top the top of the new stack area
 *
 * @note the new stack area is assumed to at least hold the old stack
 * @note remember that SPARC stacks grow from top to bottom
 *
 * @warn XXX this does to work with -O0 at the moment
 */

int stack_migrate(void *sp, void *stack_top_new)
{
	int i;

	void *sp_new;

	unsigned long stack_sz;
	unsigned long stack_top;
	unsigned long stack_bot;

	unsigned long stackf_sz;


	struct stack_trace x;
	struct sparc_stackf *frames[30];
	struct pt_regs      *regs[30];

	struct sparc_stackf *stack;



	if (!stack_top_new)
		return -EINVAL;

	/* migrate the current stack */
	if (!sp)
		sp = (void *) leon_get_sp();

	/* 30 should be more than enough */
	x.max_entries = 30;
	x.nr_entries  = 0;
	x.frames      = frames;
	x.regs        = regs;


	/* this also flushes SPARC register windows */
	save_stack_trace(&x, (uint32_t) sp, 0);


	stack_top = (unsigned long) frames[x.nr_entries - 1];
	stack_bot = (unsigned long) frames[0];

	stack_sz = stack_top - stack_bot;

	/* new bottom of stack */
	sp_new = (void *) ((char *) stack_top_new - stack_sz);

	stack = (struct sparc_stackf *) sp_new;

	/* copy the old stack */
	memcpy(sp_new, (void *) stack_bot, stack_sz);

	/* adjust frame addresses for all but top frame */
	for(i = 0; i < (x.nr_entries - 1); i++) {

		stackf_sz = (unsigned long) frames[i]->fp
			  - (unsigned long) frames[i];
		stack->fp = (void *) ((char *) stack + stackf_sz);

		stack = stack->fp;
	}

	/* go back to new bottom of stack */
	stack = (struct sparc_stackf *) sp_new;

	/* update frame pointer so we jump to the migrated stack on return */
	leon_set_fp((unsigned long) stack->fp);

	return 0;
}
