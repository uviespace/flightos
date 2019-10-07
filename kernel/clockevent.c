/**
 * @file kernel/clockevent.c
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 *
 * @ingroup time
 *
 * @note This roughly follows the concept found in linux clockevents
 *	 All glory to the Hypnotoad!
 */


#include <asm-generic/spinlock.h>
#include <asm-generic/irqflags.h>
#include <kernel/clockevent.h>
#include <kernel/export.h>
#include <kernel/tick.h>
#include <errno.h>



static LIST_HEAD(clockevent_devices);

static struct spinlock clockevents_spinlock;


/**
 * @brief convert nanoseconds delta to device ticks
 *
 * @note this implicitly clamps the delta to the valid range of the device
 */

static
unsigned long clockevents_delta2ticks(unsigned long delta,
				      struct clock_event_device *dev)
{
	delta = (unsigned long) clamp((typeof(dev->max_delta_ns)) delta,
				      dev->min_delta_ns, dev->max_delta_ns);

	return delta / dev->mult;
}


/**
 * @brief check if a timeout is in the legal range for the device
 *
 * @returns true if in range, false otherwise
 */

bool clockevents_timout_in_range(struct clock_event_device *dev,
				 unsigned long nanoseconds)
{
	unsigned long cl;

	cl = (unsigned long) clamp((typeof(dev->max_delta_ns)) nanoseconds,
				    dev->min_delta_ns, dev->max_delta_ns);

	return cl == nanoseconds;
}


/**
 * @brief check if a device supports periodic ticks
 *
 * @returns true if the feature is supported
 */

bool clockevents_feature_periodic(struct clock_event_device *dev)
{
	if (dev->features & CLOCK_EVT_FEAT_PERIODIC)
		return true;

	return false;
}


/**
 * @brief check if a device supports oneshot ticks
 *
 * @returns true if the feature is supported
 */

bool clockevents_feature_oneshot(struct clock_event_device *dev)
{
	if (dev->features & CLOCK_EVT_FEAT_ONESHOT)
		return true;

	return false;
}


/**
 * @brief check if a device supports a given state
 *
 * @returns true if a feature is supported
 *
 * @note only operative modes (periodic, oneshot are considered)
 */

bool clockevents_state_supported(struct clock_event_device *dev,
				 enum clock_event_state state)
{
	switch (state)
	{
	case CLOCK_EVT_STATE_PERIODIC:
		return clockevents_feature_periodic(dev);
	case CLOCK_EVT_STATE_ONESHOT:
		return clockevents_feature_oneshot(dev);
	default:
		break;

	}

	return true;
}


/**
 * @brief set the operating state of a clock event device
 * @param dev the device to modify
 * @param state the new state
 *
 * @note if a state is not supported according to the device features, calling
 *	 this function will have no effect
 */

void clockevents_set_state(struct clock_event_device *dev,
			   enum clock_event_state state)
{
	if (!dev) {
		pr_warn("CLOCKEVENT: NULL pointer argument to %s in call from "
			"%p\n", __func__, __caller(0));
		return;
	}


	if (!clockevents_state_supported(dev, state)) {
		pr_warn("CLOCKEVENT: selected state %d not supported by device "
			"%s\n", state, dev->name);
		return;
	}


	if (dev->state != state) {


		dev->set_state(state, dev);
		dev->state = state;
	}
}


/**
 * @brief set the event handler for a clock event device
 */

void clockevents_set_handler(struct clock_event_device *dev,
			     void (*event_handler)(struct clock_event_device *))
{
	dev->event_handler = event_handler;
}


/**
 * @brief suspend all clock devices
 */

void clockevents_suspend(void)
{
	struct clock_event_device *dev;

	list_for_each_entry_rev(dev, &clockevent_devices, node)
		if (dev->suspend)
			dev->suspend(dev);
}


/**
 *  @brief resume all clock devices
 */
void clockevents_resume(void)
{
	struct clock_event_device *dev;

	list_for_each_entry(dev, &clockevent_devices, node)
		if (dev->resume)
			dev->resume(dev);
}


/**
 * @brief release a clock event device in exchange for another one
 *
 * @param old the device to be released (may be NULL)
 * @param new the device to be acquired (may be NULL)
 */

void clockevents_exchange_device(struct clock_event_device *old,
				 struct clock_event_device *new)
{

	if (old)
		clockevents_set_state(old, CLOCK_EVT_STATE_UNUSED);


	if (new) {
		BUG_ON(new->state != CLOCK_EVT_STATE_UNUSED);
		clockevents_set_state(new, CLOCK_EVT_STATE_SHUTDOWN);
	}
}


/**
 * @brief register a clock event device
 */

void clockevents_register_device(struct clock_event_device *dev)
{

	BUG_ON(!dev);

	if (!dev->set_next_event) {
		pr_crit("set_next_event() not set for clock %p\n", dev);
		return;
	}

	if (dev->features & CLOCK_EVT_FEAT_KTIME) {
		if (!dev->set_next_ktime) {
			pr_crit("set_next_ktime() not set for clock %p\n", dev);
			return;
		}
	}

	if (!dev->set_state) {
		pr_crit("set_state() not set for clock %p\n", dev);
		return;
	}

	if (!dev->suspend)
		pr_err("suspend() not set for clock %p\n", dev);

	spin_lock(&clockevents_spinlock);
	list_add_tail(&dev->node, &clockevent_devices);

	arch_local_irq_disable();
	tick_check_device(dev);
	arch_local_irq_enable();

	spin_unlock(&clockevents_spinlock);
}
EXPORT_SYMBOL(clockevents_register_device);


/**
 * @brief if an unused clock event device is available, offer it to the ticker
 *
 * @returns 0 on success, -ENODEV if no unused device was available
 */

int clockevents_offer_device(void)
{
	struct clock_event_device *dev;

	list_for_each_entry(dev, &clockevent_devices, node) {

		if (dev->state != CLOCK_EVT_STATE_UNUSED)
			continue;

		arch_local_irq_disable();
		tick_check_device(dev);
		arch_local_irq_enable();

		return 0;
	}

	return -ENODEV;
}


/**
 * @brief program a clock event
 *
 * returns 0 on success, -ETIME if expiration time is in the past
 *
 * @warn if the timeout exceeds the bounds of the programmable range
 *	 for the device, it is forcibly clamped without warning
 *
 * @note if the clock event device is in periodic mode, the delta between
 *	 expiration time and current time will be the new period
 */

int clockevents_program_event(struct clock_event_device *dev,
			      struct timespec expires)
{
	unsigned long evt;

	double delta;


	if (dev->state == CLOCK_EVT_STATE_SHUTDOWN)
		return 0;


	/* if the set_nex_ktime handler was configured for this device */
	if (dev->features & CLOCK_EVT_FEAT_KTIME)
		return dev->set_next_ktime(expires, dev);


	/* otherwise we'll do it ourselves */
	delta = difftime_ns(expires, get_ktime());

	if (delta < 0)
		return -ETIME;

	/* clamp, adjust to clock tick period and set event */
	evt = clockevents_delta2ticks((unsigned long) delta, dev);

	dev->set_next_event(evt, dev);

	return 0;
}


/**
 * @brief program a clockevent timeout in nanoseconds
 *
 * returns 0 on success, 1 if range was clamped
 *
 * @warn if the timeout exceeds the bounds of the programmable range
 *	 for the device, it is forcibly clamped
 */

int clockevents_program_timeout_ns(struct clock_event_device *dev,
				   unsigned long nanoseconds)
{
	unsigned long evt;


	if (dev->state == CLOCK_EVT_STATE_SHUTDOWN)
		return 0;

	/* clamp and adjust to clock tick period */
	evt = clockevents_delta2ticks(nanoseconds, dev);

	dev->set_next_event(evt, dev);

	return evt != nanoseconds;
}



