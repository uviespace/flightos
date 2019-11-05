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
#include <kernel/tick.h>
#include <kernel/time.h>
#include <kernel/export.h>
#include <kernel/clockevent.h>
#include <kernel/kthread.h>
#include <kernel/irq.h>
#include <kernel/smp.h>

#include <asm/processor.h>

#define MSG "TICK: "



/* XXX CPUS!
 * maybe: enumerate CPUS, make pointer array from struct, terminate with NULL?
 */
static struct {
	ktime         prev_cal_time;
	unsigned long tick_period_min_ns;

	struct clock_event_device *dev;

} tick_device[2];


static void tick_calibrate_handler(struct clock_event_device *dev)
{
	int cpu;

	ktime now;
	ktime delta;


	cpu = smp_cpu_id();
	now = ktime_get();

	delta = ktime_delta(now, tick_device[cpu].prev_cal_time);

	if (tick_device[cpu].prev_cal_time)
		tick_device[cpu].tick_period_min_ns = (unsigned long) delta;

	tick_device[cpu].prev_cal_time = now;
}


/**
 * @brief calibrate the minimum processable tick length for this device
 *
 * what this will do eventually:
 *  - disable scheduling (maybe)
 *  - mask all interrupts except timer (maybe)
 *  - flush all caches before programming the timeoutii (we want worst-case times)
 *  - in tick_event_handler, record time between calls
 *  - keep decreasing tick length until time between calls does not decrement
 *    (i.e. interrupt response limit has been hit)...or increase...
 *  - NOTE: check clockevents_timout_in_range() or somesuch to clamp to
 *	    actual timer range (maybe add function to clockevents to
 *	    return actual timer minimum
 *  - multiply tick length by some factor (2...10)
 *  - ???
 *  - profit!
 */

static void tick_calibrate_min(struct clock_event_device *dev)
{
#define CALIBRATE_LOOPS 100

	int cpu;
	int i = 0;

	unsigned long min;
	unsigned long step;
	unsigned long tick = 0;

	ktime prev;


	cpu = smp_cpu_id();

	tick_device[cpu].tick_period_min_ns = 0;
	tick_device[cpu].prev_cal_time      = 0;

	/* we prefer one shot mode, but we'll grit our teeth, use periodic
	 * and hope for the best, if the former is not supported
	 */
	if (tick_set_mode(TICK_MODE_ONESHOT)) {
		if (tick_set_mode(TICK_MODE_PERIODIC)) {
			/* this is some weird clock device... */
			/* XXX should raise kernel alarm here */
			return;
		}
	}

	clockevents_set_handler(dev, tick_calibrate_handler);

	step = ktime_get_readout_overhead();

	prev = tick_device[cpu].prev_cal_time;

	/* This should give us the minimum tick duration on first pass unless
	 * the uptime clock has really bad resolution. If so, we'll increment
	 * the timeout by the uptime clock readout overhead and try again.
	 * This may not be as reliable if the clock device is in periodic
	 * mode, but we should still get a somewhat sensible value.
	 *
	 * Note: the minimum effective tick period is typically in the order of
	 * the interrupt processing time + some ISR overhead.
	 *
	 * XXX If there is a reboot/FDIR watchdog, make sure to enable it before
	 *     initiating tick calibration, otherwise we could get stuck here,
	 *     if the clock device does not actually function. We can't use
	 *     a timeout here to catch this, since we're obviously in the
	 *     process of initialising the very device...
	 */

	while (!tick_device[cpu].tick_period_min_ns) {

		tick += step;

		clockevents_program_timeout_ns(dev, tick);

		while (prev == tick_device[cpu].prev_cal_time)
			cpu_relax();

		barrier(); /* prevent incorrect optimisation */

		prev = tick_device[cpu].prev_cal_time;
	}

	/* ok, we found a tick timeout, let's do this a couple of times */
	min = tick_device[cpu].tick_period_min_ns;

	for (i = 1; i < CALIBRATE_LOOPS; i++) {

		/* XXX should flush caches here, especially icache */

		tick_device[cpu].tick_period_min_ns = 0;

		clockevents_program_timeout_ns(dev, tick);

		while (prev == tick_device[cpu].prev_cal_time)
			cpu_relax();

		barrier(); /* prevent incorrect optimisation */

		/* something went wrong, we'll take we got so far and bail */
		if (!tick_device[cpu].tick_period_min_ns) {

			min /= i;
			tick_device[cpu].tick_period_min_ns = min;

			/* XXX raise a kernel alarm on partial calibration */
			return;
		}

		prev = tick_device[cpu].prev_cal_time;

		min += tick_device[cpu].tick_period_min_ns;

		tick_device[cpu].tick_period_min_ns = 0;
	}

	min /= (i - 1);

	/* to avoid sampling effects, we set this to at least 2x the minimum */
	tick_device[cpu].tick_period_min_ns = min * 2;

	pr_warn(MSG "calibrated minimum timeout of tick device to %d ns\n",
		     tick_device[cpu].tick_period_min_ns);

	clockevents_set_handler(dev, NULL);
}


/**
 * @brief get the tick device for a given cpu
 */

static struct clock_event_device *tick_get_device(__attribute__((unused)) int cpu)
{
	return tick_device[cpu].dev;
}


/**
 * @brief set the tick device for a given cpu
 */

static void tick_set_device(struct clock_event_device *dev,
			    __attribute__((unused)) int cpu)
{
	tick_device[cpu].dev = dev;
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
 * @brief configure the tick device
 */

static void tick_setup_device(struct clock_event_device *dev, int cpu)
{
	irq_set_affinity(dev->irq, cpu);

	tick_calibrate_min(dev);

	/* FIXME: assume blindly for the moment, should apply mode
	 * of previous clock device (if replaced) */
	tick_set_mode_periodic(dev);
}


/**
 * @brief offer a new clock event device to the ticker
 */

void tick_check_device(struct clock_event_device *dev)
{
	int cpu;

	struct clock_event_device *cur;


	if (!dev)
		return;


	cpu = smp_cpu_id();

	cur = tick_get_device(cpu);

	if (!tick_check_preferred(cur, dev))
		return;

	clockevents_exchange_device(cur, dev);

	tick_set_device(dev, cpu);

	tick_setup_device(dev, cpu);

	/* XXX should inform scheduler to recalculate any deadline-related
	 * timeouts of tasks */
}


/**
 * @brief configure the mode of the ticker
 *
 * @returns 0 on success, -EINVAL if mode not available,
 *	   -ENODEV if no device is available for the current CPU
 */

int tick_set_mode(enum tick_mode mode)
{
	struct clock_event_device *dev;


	dev = tick_get_device(smp_cpu_id());
	if (!dev)
		return -ENODEV;

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
	return tick_device[smp_cpu_id()].tick_period_min_ns;
}


/**
 * @brief configure next tick period in nanoseconds for a cpu tick deivce
 *
 * returns 0 on success, 1 if nanoseconds range was clamped to clock range,
 *	   -ENODEV if no device is available for the selected CPU
 *
 * @note if the tick period is smaller than the calibrated minimum tick period
 *       of the timer, it will be clamped to the lower bound and a kernel alarm
 *       will be raised
 */

int tick_set_next_ns_for_cpu(unsigned long nanoseconds, int cpu)
{
	struct clock_event_device *dev;

	dev = tick_get_device(cpu);
	if (!dev)
		return -ENODEV;

	if (nanoseconds < tick_device[smp_cpu_id()].tick_period_min_ns) {
		nanoseconds = tick_device[smp_cpu_id()].tick_period_min_ns;

		/* XXX should raise kernel alarm here */
	}

	return clockevents_program_timeout_ns(dev, nanoseconds);
}


/**
 * @brief configure next tick period in nanoseconds
 *
 * returns 0 on success, 1 if nanoseconds range was clamped to clock range,
 *	   -ENODEV if no device is available for the current CPU
 */

int tick_set_next_ns(unsigned long nanoseconds)
{
	return tick_set_next_ns_for_cpu(nanoseconds, smp_cpu_id());
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


	dev = tick_get_device(smp_cpu_id());
	if (!dev)
		return -ENODEV;

	return clockevents_program_event(dev, expires);
}
