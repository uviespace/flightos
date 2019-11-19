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


#ifdef CONFIG_LEON4

/**
 * @brief start the up-counters in %asr22-23 of the LEON4
 *
 * @warn the counter apparently cannot be reset in software. this sucks big time
 *
 * @note to pause the up-counter, the MSB of asr22 of ALL processors must be set
 * @see GR740UM p 72.
 */

static uint64_t boot_time;



static inline uint64_t leon4_get_upcount(void)
{
	uint64_t time = 0;

	__asm__ __volatile__ (
			"rd	%%asr22, %H0	\n\t"
			"rd	%%asr23, %L0	\n\t"
			:"=r" (time)
			);

	time &= (0x1ULL << 56) - 1ULL;
	/* Need this for wrap-around detection. Note that 56 bits last for
	 * ~10 years @ 250 MHz, but you never know...
	 */
	if (boot_time > time) {
		/* XXX set wrap-around status (message) somewhere */
		boot_time = time;
		boot_time &= (0x1ULL << 56) - 1ULL;
	}

	return time - boot_time;
}


static void leon4_init_upcount(void)
{
	__asm__ __volatile__ (
			"wr     %%g0,    %%asr22 \n\t"
			"rd	%%asr22, %H0	\n\t"
			"rd	%%asr23, %L0	\n\t"
			:"=r" (boot_time)
			);
	boot_time &= (0x1ULL << 56) - 1ULL;
}

#endif

/* XXX I really need to rework the ktime interface so that all
 * basic times are in nanoseconds. double-word divisions area
 * REALLY expensive (doubles the overhead...)
 */
static void leon_get_uptime(uint32_t *seconds, uint32_t *nanoseconds)
{
#ifdef CONFIG_LEON3
	struct grtimer_uptime up;

	grtimer_longcount_get_uptime(grtimer_longcount, &up);
	(*seconds)     = up.coarse;
	(*nanoseconds) = CPU_CYCLES_TO_NS(up.fine);
#elif CONFIG_LEON4

	uint64_t sec;
	uint64_t nsec;


	nsec = CPU_CYCLES_TO_NS(leon4_get_upcount());

	sec = nsec /  1000000000ULL;
	/* likely faster than using modulo */
	nsec = nsec % 1000000000ULL;

	(*seconds)     = (uint32_t) sec;
	(*nanoseconds) = (uint32_t) nsec;
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


void sparc_uptime_init(void)
{
#ifdef CONFIG_LEON3
	leon_grtimer_longcount_init();
#endif

#ifdef CONFIG_LEON4
	leon4_init_upcount();
#endif

	time_init(&uptime_clock);
}
