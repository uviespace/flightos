/**
 * @file   arch/sparc/include/stacktrace.h
 */

#ifndef _SPARC_STACKTRACE_H_
#define _SPARC_STACKTRACE_H_

#include <stdint.h>
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

void die(void);
#endif

void save_stack_trace(struct stack_trace *trace, uint32_t sp, uint32_t pc);

/* part of libgloss */
void __flush_windows(void);

#endif /* _SPARC_STACKTRACE_H_ */
