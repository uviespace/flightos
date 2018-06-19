/**
 * @file arch/sparc/kernel/time.c
 */

#include <asm/time.h>
#include <kernel/time.h>
#include <kernel/printk.h>
#include <kernel/kernel.h>



/* XXX: needs proper config option (for now; AMBA PNP autodetect later...) */

#ifdef CONFIG_LEON3
#include <grtimer_longcount.h>
static struct grtimer_unit *grtimer_longcount =
			(struct grtimer_unit *) LEON3_BASE_ADDRESS_GRTIMER;



static void leon_grtimer_longcount_init(void)
{
        grtimer_longcount_start(grtimer_longcount, GRTIMER_RELOAD,
                                GRTIMER_TICKS_PER_SEC, 0xFFFFFFFFUL);
}

#endif





static void leon_get_uptime(uint32_t *seconds, uint32_t *nanoseconds)
{
#ifdef CONFIG_LEON3
	struct grtimer_uptime up;

	grtimer_longcount_get_uptime(grtimer_longcount, &up);
	(*seconds)     = up.coarse;
	(*nanoseconds) = CPU_CYCLES_TO_NS(up.fine);

#else
	printk("%s:%s not implemented\n", __FILE__, __func__);
	BUG();
#endif /* CONFIG_LEON3 */
}



static int leon_timer_enable(void)
{
	printk("%s:%s not implemented\n", __FILE__, __func__);
	BUG();
	return 0;
}


static void leon_timer_disable(void)
{
	printk("%s:%s not implemented\n", __FILE__, __func__);
	BUG();
}



/**
 * the configuration for the high-level timing facility
 */

static struct clocksource uptime_clock = {
	.read    = leon_get_uptime,
	.enable  = leon_timer_enable,
	.disable = leon_timer_disable,

};


void leon_uptime_init(void)
{
#ifdef CONFIG_LEON3
	leon_grtimer_longcount_init();
#endif
	time_init(&uptime_clock);
}
