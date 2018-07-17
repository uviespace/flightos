/**
 * @file arch/sparc/kernel/clockevent.c
 */

#include <kernel/kernel.h>
#include <kernel/clockevent.h>
#include <kernel/kmem.h>
#include <kernel/string.h>
#include <kernel/irq.h>
#include <errno.h>

#ifdef CONFIG_LEON3
#include <gptimer.h>
#include <asm/time.h>



/* XXX: want AMBA PNP autodetect later...) */


#define LEON3_GPTIMERS	4
#define GPTIMER_0_IRQ	8

static struct gpclkdevs {
	struct gptimer_unit *gptu;
	struct clock_event_device dev[LEON3_GPTIMERS];
} _gp_clk_ev = {
	.gptu =  (struct gptimer_unit *)  LEON3_BASE_ADDRESS_GPTIMER
};



/**
 * @brief the clock device event handler
 */

static irqreturn_t gp_clk_dev_irq_handler(unsigned int irq, void *userdata)
{
	struct clock_event_device *ce = (struct clock_event_device *) userdata;

	if (ce)
		if (ce->event_handler)
			ce->event_handler(ce);

	return 0;
}


/**
 * @brief clock device suspend call
 */

static void gp_clk_dev_suspend(struct clock_event_device *ce)
{
	gptimer_clear_enabled(_gp_clk_ev.gptu, ce->irq - GPTIMER_0_IRQ);

}


/**
 * @brief clock device resume call
 */

static void gp_clk_dev_resume(struct clock_event_device *ce)
{
	gptimer_set_enabled(_gp_clk_ev.gptu, ce->irq - GPTIMER_0_IRQ);
}


/**
 * @brief clock device set_state call
 */

static void gp_clk_dev_set_state(enum clock_event_state state,
				struct clock_event_device *ce)
{
	int timer;


	/* derive the timer index from its IRL */
	timer = ce->irq - GPTIMER_0_IRQ;


	switch (state) {
		case CLOCK_EVT_STATE_PERIODIC:
			gptimer_set_restart(_gp_clk_ev.gptu, timer);
			gp_clk_dev_resume(ce);
			break;
		case CLOCK_EVT_STATE_ONESHOT:
			gptimer_clear_restart(_gp_clk_ev.gptu, timer);
			gp_clk_dev_resume(ce);
			break;
		case CLOCK_EVT_STATE_SHUTDOWN:
			gp_clk_dev_suspend(ce);
			break;
		case CLOCK_EVT_STATE_UNUSED:
			gp_clk_dev_suspend(ce);
			break;
		default:
			break;
	}
}


/**
 * @brief program the gptimer to underflow at a given event timeout
 * @param evt the number of clock source ticks until the event
 *
 * @note the expiration time will be forcibly clamped to the valid range of
 *	 clock device
 */

static int gp_clk_dev_set_next_event(unsigned long evt,
				     struct clock_event_device *ce)
{
	int timer;


	/* derive the timer index from its IRL */
	timer = ce->irq - GPTIMER_0_IRQ;

	switch(ce->state) {
		case CLOCK_EVT_STATE_PERIODIC:
			gptimer_start_cyclical(_gp_clk_ev.gptu, timer, evt);
			break;
		case CLOCK_EVT_STATE_ONESHOT:
			gptimer_start(_gp_clk_ev.gptu, timer, evt);
			break;
		default:
			break; /* not sure what you want? */
	};


	return 0;
}


/**
 * @brief program the gptimer to underflow at a given absolute kernel time
 *
 * @param ktime the kernel time at which the timer will underflow
 *
 *
 * @returns 0 on success, -ETIME if time is in the past
 *
 * @note the expiration time will be forcibly clamped to the valid range of
 *	 clock device
 *
 */

static int gp_clk_dev_set_next_ktime(struct timespec expires,
				     struct clock_event_device *ce)
{
	uint32_t evt;

	double delta;


	delta = difftime_ns(expires, get_ktime());

	if (delta < 0)
		return -ETIME;

	/* clamp to valid range */
	evt  = clamp((typeof(ce->max_delta_ns)) delta,
		     ce->min_delta_ns, ce->max_delta_ns);

	/* adjust ns delta to actual clock tick value*/
	evt = evt / ce->mult;

	return gp_clk_dev_set_next_event((unsigned long) evt, ce);
}


/**
 * @brief register the 4 general purpose timers (of the GR712)
 */

static void leon_setup_clockdevs(void)
{
	int i;
	char *buf;

	struct clock_event_device *ce;


	/* the prescaler is the same for all timers */
	gptimer_set_scaler_reload(_gp_clk_ev.gptu, GPTIMER_RELOAD);

	for (i = 0; i < LEON3_GPTIMERS; i++) {


		ce = &_gp_clk_ev.dev[i];

		ce->mult = CPU_CYCLES_TO_NS(GPTIMER_RELOAD + 1);

		/* we can only fit so many nanoseconds into a 32 bit number
		 * note that we set the timer value in "ticks", where each tick
		 * corresponds to GPTMER_RELOAD + 1 cpu cycles, so our actual
		 * possible timeout in nanoseconds is typically much higher
		 * than what we can fit in here
		 */

		if (sizeof(typeof(ce->max_delta_ns)) > 4)
			ce->max_delta_ns = 0xFFFFFFFFUL * ce->mult;
		else
			ce->max_delta_ns = 0xFFFFFFFFUL;

		/* We cannot be better than the system clock overhead,
		 * as long as we support CLOCK_EVT_FEAT_KTIME
		 * To be safe, we should at least use twice that value.
		 */
		ce->min_delta_ns = 2 * ktime_get_readout_overhead();

		BUG_ON(!ce->mult);

		ce->set_next_event = &gp_clk_dev_set_next_event;
		ce->set_next_ktime = &gp_clk_dev_set_next_ktime;
		ce->set_state      = &gp_clk_dev_set_state;
		ce->suspend        = &gp_clk_dev_suspend;
		ce->resume         = &gp_clk_dev_resume;


		buf = kmalloc(16);
		BUG_ON(!buf);
		snprintf(buf, 16, "GPTIMER%1d", i);
		ce->name = buf;


		/* we rate our timers by nanosecond resoltion, so we'll just
		 * the multiplier as our rating
		 */
		ce->rating = ce->mult;

		ce->state = CLOCK_EVT_STATE_UNUSED;

		ce->features = (CLOCK_EVT_FEAT_PERIODIC |
				CLOCK_EVT_FEAT_ONESHOT  |
				CLOCK_EVT_FEAT_KTIME);


		ce->irq  = GPTIMER_0_IRQ + i;

		gptimer_clear_enabled(_gp_clk_ev.gptu, i);

		BUG_ON(irq_request(ce->irq, ISR_PRIORITY_NOW,
				   &gp_clk_dev_irq_handler,
				   &_gp_clk_ev.dev[i]));

		clockevents_register_device(ce);
	}
}
#endif /* CONFIG_LEON3 */


void sparc_clockevent_init(void)
{
#ifdef CONFIG_LEON3
	leon_setup_clockdevs();
#endif /* CONFIG_LEON3 */
}
