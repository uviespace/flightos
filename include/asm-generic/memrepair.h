/**
 * @file include/asm-generic/memrepair.h
 */



#ifndef _ASM_GENERIC_MEMREPAIR_H_
#define _ASM_GENERIC_MEMREPAIR_H_

#include <asm/memrepair.h>

#ifndef __mem_repair
#define __mem_repair __mem_repair
/**
 * @brief generic no-op memory repair fallback (self-assign the memory word)
 * @param addr: address of the memory word to repair
 */
static void mem_repair(void *addr)
{
	(* (unsigned long *)addr) = (* (unsigned long *)addr);
}
#endif

#endif	/* _ASM_GENERIC_MEMREPAIR_H_ */
