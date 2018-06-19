/**
 * @file kernel/time.c
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 *
 * @ingroup time
 * @defgroup time time interface
 *
 */


#include <errno.h>
#include <kernel/time.h>
#include <kernel/export.h>

static struct timekeeper tk;




/**
 * @brief get the time elapsed since boot
 *
 * @param[out] ts a struct timespec
 *
 * @note if no uptime clock was configured, the result
 *	 will be undefined
 */

void time_get_uptime(struct timespec *ts)
{
	uint32_t sec;
	uint32_t nsec;


	if (!tk.clock)
		return;


	tk.clock->read(&sec, &nsec);

	/* We'll get away with this as long as we exist in 32-bit space, since
	 * the members of struct timespec are usually of long int type.
	 * (see also kernel/time.h)
	 */

	ts->tv_sec  = (typeof(ts->tv_sec)) sec;
	ts->tv_nsec = (typeof(ts->tv_sec)) nsec;



}
EXPORT_SYMBOL(time_get_uptime);



/**
 * @brief initialise the timing system
 */

void time_init(struct clocksource *clock)
{
	tk.clock = clock;
}
