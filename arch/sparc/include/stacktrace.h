/**
 * @file   arch/sparc/include/stacktrace.h
 */

#ifndef _SPARC_STACKTRACE_H_
#define _SPARC_STACKTRACE_H_

#include <kernel/types.h>
#include <stack.h>

struct stack_trace {
        uint32_t nr_entries;
	uint32_t max_entries;
        struct sparc_stackf **frames;
        struct pt_regs      **regs;
};


#if defined(USE_STACK_TRACE_TRAP)
/**
 * @brief a trap handler to execute a stack trace (implemented in asm)
 */
void trace_trap(void);
#endif

/**
 * @brief performs a stack trace
 *
 * @param trace a struct stack_trace
 * @param sp a stack/frame pointer
 * @param pc a program counter
 *
 * @note when being called from a trap, the pc in %o7 is NOT the return program
 *	 counter of the trapped function, so a stack/frame pointer by itself
 *	 is not enough to provide a proper trace, hence the pc argument
 */
void save_stack_trace(struct stack_trace *trace, uint32_t sp, uint32_t pc);

/**
 * @brief flush the SPARC register windows to memory
 *
 * @note part of libgloss
 */
void __flush_windows(void);

#endif /* _SPARC_STACKTRACE_H_ */
