/**
 * @file    include/kernel/clockevent.h
 * @ingroup timing
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @brief High-level clockevent device abstraction and management interface.
 *
 * @ingroup time
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
 */

#ifndef _KERNEL_CLOCKEVENT_H_
#define _KERNEL_CLOCKEVENT_H_

#include <list.h>
#include <kernel/types.h>
#include <kernel/time.h>


/**
 * @brief clock event device states
 */
enum clock_event_state {
	CLOCK_EVT_STATE_UNUSED,		/*!< device not in use */
	CLOCK_EVT_STATE_SHUTDOWN,	/*!< device is shut down */
	CLOCK_EVT_STATE_PERIODIC,	/*!< device in periodic mode */
	CLOCK_EVT_STATE_ONESHOT,	/*!< device in one-shot mode */
	CLOCK_EVT_STATE_WATCHDOG	/*!< device in watchdog mode */
};


/* feature set of a particular clock device */

/** @brief device supports periodic mode */
#define CLOCK_EVT_FEAT_PERIODIC	0x000001
/** @brief device supports one-shot mode */
#define CLOCK_EVT_FEAT_ONESHOT	0x000002
/** @brief device supports direct ktime programming */
#define CLOCK_EVT_FEAT_KTIME	0x000004
/** @brief device can serve as a watchdog */
#define CLOCK_EVT_FEAT_WATCHDOG	0x000008


/**
 * event_handler:	callback function executed as the event occurs
 *
 * set_next_event:	set next event function using a clock source delta
 * set_next_ktime:	set next event function using a direct ktime value
 *
 * max_delta_ns:	maximum programmable delta value in nanoseconds
 * min_delta_ns:	minimum programmable delta value in nanoseconds
 * mult:		device ticks to nanoseconds multiplier
 * state:		timer operating state
 * features:		timer event features
 * set_state:		set state function
 * rating:		quality rating of the device, less is better (more
 *			resolution, e.g nanosecond-resolution)
 * name:		clock event name
 * irq:			IRQ number (-1 if device without IRL)
 */

struct clock_event_device {
	void			(*event_handler)(struct clock_event_device *);
	int			(*set_next_event)(unsigned long evt,
						  struct clock_event_device *);
	int			(*set_next_ktime)(struct timespec expires,
						  struct clock_event_device *);
	uint32_t		max_delta_ns;
	uint32_t		min_delta_ns;
	uint32_t		mult;


	enum clock_event_state	state;
	unsigned int		features;

	void			(*set_state)(enum clock_event_state state,
					    struct clock_event_device *);
	void			(*suspend)(struct clock_event_device *);
	void			(*resume)(struct clock_event_device *);

	unsigned int		rating;
	const char		*name;
	int			irq;

	struct list_head	node;
};


/**
 * @brief check if a timeout value is within the device's supported range
 * @param dev: the clock event device
 * @param nanoseconds: timeout value to check
 * @return true if in range, false otherwise
 */
bool clockevents_timout_in_range(struct clock_event_device *dev,
				 unsigned long nanoseconds);

/**
 * @brief check if a device supports periodic mode
 * @param dev: the clock event device
 * @return true if periodic mode is supported
 */
bool clockevents_feature_periodic(struct clock_event_device *dev);

/**
 * @brief check if a device supports one-shot mode
 * @param dev: the clock event device
 * @return true if one-shot mode is supported
 */
bool clockevents_feature_oneshot(struct clock_event_device *dev);

/**
 * @brief check if a device supports watchdog mode
 * @param dev: the clock event device
 * @return true if watchdog mode is supported
 */
bool clockevents_feature_watchdog(struct clock_event_device *dev);

/**
 * @brief set the state of a clock event device
 * @param dev: the clock event device
 * @param state: desired state
 */
void clockevents_set_state(struct clock_event_device *dev,
			   enum clock_event_state state);

/**
 * @brief set the event handler callback for a clock event device
 * @param dev: the clock event device
 * @param event_handler: callback function for timer events
 */
void clockevents_set_handler(struct clock_event_device *dev,
			     void (*event_handler)(struct clock_event_device *));

/**
 * @brief register a clock event device with the clockevent subsystem
 * @param dev: the clock event device to register
 */
void clockevents_register_device(struct clock_event_device *dev);

/**
 * @brief offer a clock event device for use by the tick subsystem
 * @return 0 on success, negative error code on failure
 */
int clockevents_offer_device(void);

/**
 * @brief exchange the active clock event device
 * @param old: the device to replace (or NULL)
 * @param new: the new device to activate (or NULL)
 */
void clockevents_exchange_device(struct clock_event_device *old,
				 struct clock_event_device *new);

/**
 * @brief program a clock event device to fire at an absolute time
 * @param dev: the clock event device
 * @param expires: absolute time of the event
 * @return 0 on success, negative error code on failure
 */
int clockevents_program_event(struct clock_event_device *dev,
			      struct timespec expires);

/**
 * @brief program a clock event device to fire after a relative timeout
 * @param dev: the clock event device
 * @param nanoseconds: timeout in nanoseconds from now
 * @return 0 on success, negative error code on failure
 */
int clockevents_program_timeout_ns(struct clock_event_device *dev,
				   unsigned long nanoseconds);



#endif /* _KERNEL_CLOCKEVENT_H_ */
