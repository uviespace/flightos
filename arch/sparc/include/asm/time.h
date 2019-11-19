/**
 * @file arch/sparc/include/asm/time.h
 */

#ifndef _SPARC_TIME_H_
#define _SPARC_TIME_H_


#include <kernel/kernel.h>


#define SPARC_CPU_CPS		0

#ifdef CONFIG_CPU_CLOCK_FREQ
#undef SPARC_CPU_CPS
#define SPARC_CPU_CPS		CONFIG_CPU_CLOCK_FREQ
#endif /* CONFIG_CPU_CLOCK_FREQ */

#if !SPARC_CPU_CPS
#error CPU clock frequency not configured and no detection method available.
#endif


#define GPTIMER_RELOAD		7
#define GRTIMER_RELOAD		4

#define GPTIMER_TICKS_PER_SEC	((SPARC_CPU_CPS / (GPTIMER_RELOAD + 1)))
#define GPTIMER_TICKS_PER_MSEC	(GPTIMER_TICKS_PER_SEC / 1000)
#define GPTIMER_TICKS_PER_USEC	(GPTIMER_TICKS_PER_SEC / 1000000)
#define GPTIMER_USEC_PER_TICK	(1000000.0 / GPTIMER_TICKS_PER_SEC)

#define GRTIMER_TICKS_PER_SEC	((SPARC_CPU_CPS / (GRTIMER_RELOAD + 1)))
#define GRTIMER_TICKS_PER_MSEC	(GRTIMER_TICKS_PER_SEC / 1000)
#define GRTIMER_TICKS_PER_USEC	(GRTIMER_TICKS_PER_SEC / 1000000)
#define GRTIMER_USEC_PER_TICK	(1000000.0 / GRTIMER_TICKS_PER_SEC)

#define GPTIMER_CYCLES_PER_SEC	SPARC_CPU_CPS
#define GPTIMER_CYCLES_PER_MSEC	(GPTIMER_CYCLES_PER_SEC / 1000)
#define GPTIMER_CYCLES_PER_USEC	(GPTIMER_CYCLES_PER_SEC / 1000000)
#define GPTIMER_USEC_PER_CYCLE	(1000000.0 / GPTIMER_CYCLES_PER_SEC)

#define GRTIMER_CYCLES_PER_SEC	SPARC_CPU_CPS
#define GRTIMER_CYCLES_PER_MSEC	(GRTIMER_CYCLES_PER_SEC / 1000)
#define GRTIMER_CYCLES_PER_USEC	(GRTIMER_CYCLES_PER_SEC / 1000000)
#define GRTIMER_SEC_PER_CYCLE	(      1.0 / GRTIMER_CYCLES_PER_SEC)
#define GRTIMER_MSEC_PER_CYCLE	(   1000.0 / GRTIMER_CYCLES_PER_SEC)
#define GRTIMER_USEC_PER_CYCLE	(1000000.0 / GRTIMER_CYCLES_PER_SEC)

/* yeah...need to work on that ...*/
#if defined (CONFIG_LEON4)

#define CPU_CYCLES_TO_NS(x) ((x) * (1000000000 / SPARC_CPU_CPS))

#else
/* this will definitely break if we run at GHz clock speeds
 * note that the order is important, otherwise we may encounter integer
 * overflow on multiplication
 */
#define CPU_CYCLES_TO_NS(x) (((x) >= 1000UL) \
			     ? (((x) / (SPARC_CPU_CPS / 1000000UL)) * 1000UL) \
			     : (((x) * 1000UL) / (SPARC_CPU_CPS / 1000000UL)))
#endif

compile_time_assert((SPARC_CPU_CPS <= 1000000000UL),
		    CPU_CYCLES_TO_NS_NEEDS_FIXUP);





void sparc_uptime_init(void);

#endif /* _SPARC_TIME_H_ */
