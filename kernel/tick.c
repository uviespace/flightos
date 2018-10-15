/**
 * @file kernel/tick.c
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 *
 * @ingroup time
 *
 * @note this roughly follows the concept found in linux ticks
 */



#include <errno.h>
#include <kernel/tick.h>
#include <kernel/time.h>
#include <kernel/export.h>
#include <kernel/clockevent.h>
#include <kernel/kthread.h>

#define MSG "TICK: "

/* the minimum effective tick period; default to 1 ms */
static unsigned long tick_period_min_ns = 1000000UL;

static struct clock_event_device *tick_device;




static void tick_event_handler(struct clock_event_device *dev)
{
	/* does nothing, schedule later */
}


struct clock_event_device *tick_get_device(__attribute__((unused)) int cpu)
{
	return tick_device;
}

void tick_set_device(struct clock_event_device *dev,
					   __attribute__((unused)) int cpu)
{
	tick_device = dev;
}

/**
 * @brief tick device selection check
 *
 * @note placeholder, does not do much right now
 */

static bool tick_check_preferred(struct clock_event_device *cur,
				 struct clock_event_device *new)
{

	/* XXX: need that until we have internal mode tracking for the
	 * ticker, after wich we can reprogram the the oneshot
	 * timer after each event to emulate periodicity
	 */
	if (!clockevents_feature_periodic(new))
		return false;


	/* If we have nothing, we'll take what we can get */
	if (!cur)
		return true;

	return false;
}


/**
 * @brief configure for periodic mode if available
 *
 * @returns -EINVAL if mode is not supported by underlying clock event device
 */

static int tick_set_mode_periodic(struct clock_event_device *dev)
{
	if (!clockevents_feature_periodic(dev))
		return -EINVAL;

	clockevents_set_state(dev, CLOCK_EVT_STATE_PERIODIC);

	return 0;
}


/**
 * @brief configure for oneshot mode if available
 *
 * @returns -EINVAL if mode is not supported by underlying clock event device
 */

static int tick_set_mode_oneshot(struct clock_event_device *dev)
{
	if (!clockevents_feature_oneshot(dev))
		return -EINVAL;

	clockevents_set_state(dev, CLOCK_EVT_STATE_ONESHOT);

	return 0;
}

/**
 * @brief calibrate the minimum processable tick length for this device
 *
 * XXX:
 * what this will do:
 *  - disable scheduling
 *  - mask all interrupts except timer (maybe)
 *  - in tick_event_handler, record time between calls
 *  - keep decreasing tick length until time between calls does not decrement
 *    (i.e. interrupt response limit has been hit)
 *  - NOTE: check clockevents_timout_in_range() or somesuch to clamp to
 *	    actual timer range (maybe add function to clockevents to
 *	    return actual timer minimum
 *  - multiply tick length by some factor (2...10)
 *  - ???
 *  - profit!
 */

static void tick_calibrate_min(struct clock_event_device *dev)
{
#define RANDOM_TICK_RATE_NS	18000UL
	tick_period_min_ns = RANDOM_TICK_RATE_NS;
#define MIN_SLICE		1000000UL
	tick_period_min_ns = MIN_SLICE;
}


/**
 * @brief configure the tick device
 */

static void tick_setup_device(struct clock_event_device *dev)
{
	tick_calibrate_min(dev);

	/* FIXME: assume blindly for the moment, should apply mode
	 * of previous clock device (if replaced) */
	tick_set_mode_periodic(dev);

	clockevents_set_handler(dev, tick_event_handler);
	clockevents_program_timeout_ns(dev, tick_period_min_ns);
}


/**
 * @brief offer a new clock event device to the ticker
 */

void tick_check_device(struct clock_event_device *dev)
{
	struct clock_event_device *cur;


	if (!dev)
		return;

	/* XXX need per-cpu selection later */
	cur = tick_get_device(0);

	if (!tick_check_preferred(cur, dev))
		return;

	clockevents_exchange_device(cur, dev);

	/* XXX as above */
	tick_set_device(dev, 0);

	tick_setup_device(dev);

	/* XXX should inform scheduler to recalculate any deadline-related
	 * timeouts of tasks */
}


/**
 * @brief configure the mode of the ticker
 *
 * @returns 0 on success, -EINVAL if mode not available
 */

int tick_set_mode(enum tick_mode mode)
{
	struct clock_event_device *dev;


	/* XXX need per-cpu selection later */
	dev = tick_get_device(0);

	switch(mode) {
		case TICK_MODE_PERIODIC:
			return tick_set_mode_periodic(dev);
		case TICK_MODE_ONESHOT:
			return tick_set_mode_oneshot(dev);
		default:
			break;
	}

	return -EINVAL;
}


/**
 * @brief get the (calibrated) minimum effective tick period
 *
 * @note This represents a safe-to-use value of the best-effort interrupt
 *	 processing time of the system.
 */

unsigned long tick_get_period_min_ns(void)
{
	return tick_period_min_ns;
}



/**
 * @brief configure next tick period in nanoseconds
 *
 * returns 0 on success, 1 if nanoseconds range was clamped to clock range
 */


int tick_set_next_ns(unsigned long nanoseconds)
{
	struct clock_event_device *dev;


	/* XXX need per-cpu selection later */
	dev = tick_get_device(0);

	return clockevents_program_timeout_ns(dev, nanoseconds);
}


/**
 * @brief configure next tick period in ktime
 *
 * returns 0 on success, -ETIME if expiration time is in the past
 *
 * @warn if the timeout exceeds the bounds of the programmable range
 *	 for the device, it is forcibly clamped without warning
 *
 * @note if the clock event device is in periodic mode, the delta between
 *	 expiration time and current time will be the new period
 */

int tick_set_next_ktime(struct timespec expires)
{
	struct clock_event_device *dev;


	/* XXX need per-cpu selection later */
	dev = tick_get_device(0);

	return clockevents_program_event(dev, expires);
}
