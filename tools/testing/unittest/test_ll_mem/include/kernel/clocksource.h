/**
 * @file include/kernel/clocksource.h
 */

#ifndef _KERNEL_BOOTMEM_H_
#define _KERNEL_BOOTMEM_H_


#include <kernel/types.h>


struct clocksource {

	void (*read)(uint32_t *seconds, uint32_t *nanoseconds);

	int (*enable)(void);
	void (*disable)(void);
};



#endif /* _KERNEL_BOOTMEM_H_ */

