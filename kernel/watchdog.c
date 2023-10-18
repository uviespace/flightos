/**
 * @file kernel/tick.c
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 *
 * @ingroup time
 *
 * per-cpu tick device
 *
 * @note this roughly follows the concept found in linux ticks
 */



#include <errno.h>
#include <kernel/time.h>
#include <kernel/export.h>
#include <kernel/clockevent.h>
#include <kernel/watchdog.h>
#include <kernel/irq.h>

#include <asm/processor.h>

#define MSG "WDOG: "



static struct {
	ktime         prev_cal_time;
	unsigned long watchdog_period_min_ns;

	struct clock_event_device *dev;

	void (*handler)(void *);
	void *data;

} wd_dev;




/**
 * @brief get the watchdog device
 */

static struct clock_event_device *watchdog_get_device(void)
{
	return wd_dev.dev;
}


/**
 * @brief set the watchdog device
 */

static void watchdog_set_device(struct clock_event_device *dev)
{
	wd_dev.dev = dev;
}


/**
 * @brief set the watchdog bark user handler
 */

static void watchdog_set_handler_internal(void (*handler)(void *), void *data)
{
	wd_dev.handler = handler;
	wd_dev.data    = data;
}


static void watchdog_handler_internal(struct clock_event_device *dev)
{
	if (wd_dev.handler)
		wd_dev.handler(wd_dev.data);
}



/**
 * @brief tick device selection check
 *
 * @note placeholder, does not do much right now
 */

static bool watchdog_check_preferred(struct clock_event_device *cur,
				 struct clock_event_device *new)
{
	/* need a watchdog feature, else this is pointless */
	if (!clockevents_feature_watchdog(new))
		return false;

	/* If we have nothing, we'll take what we can get */
	if (!cur)
		return true;

	return false;
}


/**
 * @brief configure the tick device
 */

static void watchdog_setup_device(struct clock_event_device *dev)
{
	/* XXX we may want to make this configurable, for now we handle
	 * IRQs on the primary cpu
	 */
	irq_set_affinity(dev->irq, 0);

	clockevents_set_state(dev, CLOCK_EVT_STATE_WATCHDOG);

	clockevents_set_handler(dev, watchdog_handler_internal);
}


/**
 * @brief offer a new clock event device to the watchdog
 */

void watchdog_check_device(struct clock_event_device *dev)
{
	struct clock_event_device *cur;


	if (!dev)
		return;


	cur = watchdog_get_device();

	if (!watchdog_check_preferred(cur, dev))
		return;

	printk("WD GOT ONE!!!\n");

	clockevents_exchange_device(cur, dev);

	watchdog_set_device(dev);

	watchdog_setup_device(dev);
}


/**
 * @brief configure the mode of the watchdog
 *
 * @returns 0 on success, -EINVAL if mode not available,
 *	   -ENODEV if no device is available for the current CPU
 */

int watchdog_set_mode(enum watchdog_mode mode)
{
	struct clock_event_device *dev;


	dev = watchdog_get_device();
	if (!dev)
		return -ENODEV;

	switch(mode) {
		case WATCHDOG_UNLEASH:
			clockevents_set_state(dev, CLOCK_EVT_STATE_WATCHDOG);
			return 0;
		case WATCHDOG_LEASH:
			clockevents_set_state(dev, CLOCK_EVT_STATE_SHUTDOWN);
			return 0;
		default:
			break;
	}

	return -EINVAL;
}


/**
 * @brief set a handler for when the watchdog barks
 */

void watchdog_set_handler(void (*handler)(void *), void *userdata)
{
	watchdog_set_handler_internal(handler, userdata);
}


/**
 * @brief configure next watchdog timeout period in nanoseconds
 *
 * @param nanoseconds the time until the watchdog will bark;
 *	  make sure to feed the watchdog before it causes a ruckus
 *
 * @note the watchdog will stay dorment until fed for the first time
 *
 * returns 0 on success, 1 if nanoseconds range was clamped to clock range,
 *	   -ENODEV if no device is available
 */

int watchdog_feed(unsigned long nanoseconds)
{
	struct clock_event_device *dev;

	dev = watchdog_get_device();
	if (!dev)
		return -ENODEV;

	return clockevents_program_timeout_ns(dev, nanoseconds);
}
