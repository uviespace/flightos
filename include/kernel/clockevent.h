/**
 * @file    include/kernel/clockevent.h
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
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


/* clock event states */
enum clock_event_state {
	CLOCK_EVT_STATE_UNUSED,
	CLOCK_EVT_STATE_SHUTDOWN,
	CLOCK_EVT_STATE_PERIODIC,
	CLOCK_EVT_STATE_ONESHOT
};


/* feature set of a particular clock device */

#define CLOCK_EVT_FEAT_PERIODIC	0x000001
#define CLOCK_EVT_FEAT_ONESHOT	0x000002
#define CLOCK_EVT_FEAT_KTIME	0x000004


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


bool clockevents_timout_in_range(struct clock_event_device *dev,
				 unsigned long nanoseconds);

bool clockevents_feature_periodic(struct clock_event_device *dev);
bool clockevents_feature_oneshot(struct clock_event_device *dev);

void clockevents_set_state(struct clock_event_device *dev,
			   enum clock_event_state state);

void clockevents_set_handler(struct clock_event_device *dev,
			     void (*event_handler)(struct clock_event_device *));

void clockevents_register_device(struct clock_event_device *dev);

int clockevents_offer_device(void);

void clockevents_exchange_device(struct clock_event_device *old,
				 struct clock_event_device *new);

int clockevents_program_event(struct clock_event_device *dev,
			      struct timespec expires);

int clockevents_program_timeout_ns(struct clock_event_device *dev,
				   unsigned long nanoseconds);



#endif /* _KERNEL_CLOCKEVENT_H_ */
