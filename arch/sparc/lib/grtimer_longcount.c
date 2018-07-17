/**
 * @file   arch/sparc/lib/grtimer_longcount.c
 * @ingroup time
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   July, 2016
 *
 * @copyright GPLv2
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @brief implements a long-counting (uptime) clock using the LEON3 GRTIMER
 *
 */


#include <asm/io.h>
#include <time.h>
#include <grtimer.h>
#include <grtimer_longcount.h>




/**
 * @brief enable long count timer
 * @param rtu a struct grtimer_unit
 * @param reload a scaler reload value
 * @param fine_ticks_per_sec a timer reload value in ticks per second
 * @param coarse_ticks_max a timer reload value in ticks per second
 *
 * If properly configured, grtimer[0] will hold fractions of a second and
 * grtimer[1] will be in seconds, counting down from coarse_ticks_max
 *
 * @return -1 if fine_ticks_per_sec is not an integer multiple of scaler_reload,
 *          0 otherwise
 *
 * @note the return value warns about a configuration error, but will still
 *	 accept the input
 */


int32_t grtimer_longcount_start(struct grtimer_unit *rtu,
				uint32_t scaler_reload,
				uint32_t fine_ticks_per_sec,
				uint32_t coarse_ticks_max)
{
	grtimer_set_scaler_reload(rtu, scaler_reload);
	grtimer_set_reload(rtu, 0, fine_ticks_per_sec);
	grtimer_set_reload(rtu, 1, coarse_ticks_max);

	grtimer_set_load(rtu, 0);
	grtimer_set_load(rtu, 1);

	grtimer_set_restart(rtu, 0);
	grtimer_set_restart(rtu, 1);

	grtimer_set_chained(rtu, 1);

	grtimer_set_enabled(rtu, 0);
	grtimer_set_enabled(rtu, 1);

	grtimer_enable_latch(rtu);

	/* not an integer multiple, clock will drift */
	if (fine_ticks_per_sec % scaler_reload)
		return -1;

	return 0;
}


/**
 * @brief get the time since the long counting grtimer was started
 * @param rtu a struct grtimer_unit
 * @param up a struct grtimer_uptime
 * @note  if configured properly, fine will be in cpu cycles and coarse will
 *        be in seconds
 *
 *        XXX: sucks, need latching
 */

void grtimer_longcount_get_uptime(struct grtimer_unit *rtu,
				  struct grtimer_uptime *up)
{
	uint32_t sc;
	uint32_t t0, t0a, t0b, t0c;
	uint32_t t1, t1a, t1b, t1c;
	uint32_t r0;
	uint32_t r1;


	sc = ioread32be(&rtu->scaler_reload);

	t0a = ioread32be(&rtu->timer[0].value);
	t1a = ioread32be(&rtu->timer[1].value);

	t0b = ioread32be(&rtu->timer[0].value);
	t1b = ioread32be(&rtu->timer[1].value);

	t0c = ioread32be(&rtu->timer[0].value);
	t1c = ioread32be(&rtu->timer[1].value);

	if ((t0a >= t0b) && (t1a >= t1b))
	  {
	    t0 = t0a;
	    t1 = t1a;
	  }
	else
	  {
	    t0 = t0c;
	    t1 = t1c;
	  }

	r0 = ioread32be(&rtu->timer[0].reload);
	r1 = ioread32be(&rtu->timer[1].reload);

	up->fine   = (r0 - t0) * (sc + 1);
	up->coarse = (r1 - t1);
}


/**
 * @brief
 *	get the number of seconds elapsed between timestamps taken from the
 *	longcount timer
 *
 * @brief param time1 a struct grtime_uptime
 * @brief param time0 a struct grtime_uptime
 *
 * @return time difference in seconds represented as double
 */

double grtimer_longcount_difftime(struct grtimer_unit *rtu,
				  struct grtimer_uptime time1,
				  struct grtimer_uptime time0)
{
	uint32_t sc;
	uint32_t rl;
	double cpu_freq;

	double t0;
	double t1;


	sc = grtimer_get_scaler_reload(rtu);
	rl = grtimer_get_reload(rtu, 0);

	cpu_freq = (double) (sc + 1) * rl;

	t0 = (double) time0.coarse + (double) time0.fine / cpu_freq;
	t1 = (double) time1.coarse + (double) time1.fine / cpu_freq;

	return t1 - t0;
}




/**
 * @brief get the time since last latch occured in cpu cycles
 * @param rtu a struct grtimer_unit
 * @note does not compensate for function overhead
 */

uint32_t grtimer_longcount_get_latch_time_diff(struct grtimer_unit *rtu)
{
	uint32_t t0;
	uint32_t t1;

	uint32_t t0_latch;
	uint32_t t1_latch;

	uint32_t t0_reload;
	uint32_t t0_scaler;

	uint32_t diff;



	t0_latch  = grtimer_get_latch_value(rtu, 0);
	t1_latch  = grtimer_get_latch_value(rtu, 1);
	t0_reload = grtimer_get_reload(rtu, 0);
	t0_scaler = grtimer_get_scaler_reload(rtu);

	t0 = grtimer_get_value(rtu, 0);
	t1 = grtimer_get_value(rtu, 1);

	diff = (t1_latch - t1) * t0_reload * (t0_scaler + 1);

	if (t0 < t0_latch)
		diff += (t0_latch  - t0);
	else
		diff += (t0_reload - t0);


	return diff;
}
