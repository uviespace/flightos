/**
 * @file include/kernel/clocksource.h
 * @ingroup timing
 *
 * @brief High-level clocksource abstraction for reading elapsed time.
 *
 * Describes a hardware-sourced clock used to read the current time from
 * hardware and provide enable/disable operations.
 */

#ifndef _KERNEL_CLOCKSOURCE_H_
#define _KERNEL_CLOCKSOURCE_H_


#include <kernel/types.h>


/**
 * @brief clocksource structure providing hardware time reading
 */
struct clocksource {

	void (*read)(uint32_t *seconds, uint32_t *nanoseconds);	/*!< read current time from hardware */

	int (*enable)(void);	/*!< enable the clocksource, returns 0 on success */
	void (*disable)(void);	/*!< disable the clocksource */
};



#endif /* _KERNEL_CLOCKSOURCE_H_ */
