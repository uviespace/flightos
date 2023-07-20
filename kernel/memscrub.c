/**
 * @file   memscrub.c
 * @ingroup memscrub
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 * @date   March, 2016
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
 * XXX TODO sysctl IF to set scrub sections
 */


#include <errno.h>
#include <list.h>
#include <compiler.h>
#include <asm-generic/io.h>

#include <kernel/init.h>
#include <kernel/edac.h>
#include <kernel/kmem.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/sysctl.h>
#include <kernel/export.h>
#include <kernel/kthread.h>

/* if we need more space, this is how many entries we will add */
#define SCRUB_REALLOC 10

struct memscrub_sec {
	unsigned long begin;
	unsigned long end;
	unsigned long pos;
	uint16_t wpc;
};
/* this is where we keep track of scrubbing sections */
static struct {
	struct memscrub_sec *sec;
	int    sz;
	int    cnt;
} _scrub;


/**
 * @brief memory scrubbing of a range addr[0] ... addr[n] at a time
 *
 * @param addr the address pointer to start scrubbing from
 * @param n    the number of data words to scrub on top of starting address
 *
 * @return the next unscrubbed address
 */

static unsigned long memscrub(unsigned long addr, size_t n)
{
	unsigned long stop = addr + n * sizeof(unsigned long);



	for ( ; addr < stop; addr += sizeof(unsigned long))
		ioread32be((void *)addr);

	return stop;
}


/**
 * @brief scrubbing thread function
 */

static int mem_do_scrub(void *data)
{
	size_t n;
	size_t i;

	unsigned long begin;
	unsigned long end;
	unsigned long pos;

	while (1) {

		for (i= 0; i < _scrub.cnt; i++) {
			begin = _scrub.sec[i].begin;
			end   = _scrub.sec[i].end;
			pos   = _scrub.sec[i].pos;

			if (pos > end)
				pos = begin;

			n = (end - pos) / sizeof(unsigned long);

			if (n < _scrub.sec[i].wpc) {
				pos = memscrub(pos, n);
				n = _scrub.sec[i].wpc - n;
			} else {
				n = _scrub.sec[i].wpc;
			}

			_scrub.sec[i].pos = memscrub(pos, n);

		}

		sched_yield();
	}

	return 0;
}


/**
 * @brief add a memory segment to scrub
 *
 * @param being	the start address
 * @param end   the end address
 * @param len	the number of words to scrub per scrubbing cycle
 *
 * @note the addresses must be word-aligned
 *
 * @returns 0 on success, otherwise error
 */

int memscrub_seg_add(unsigned long begin, unsigned long end, uint16_t wpc)
{
	if (begin & 0x3)
		goto error;

	if (end & 0x3)
		goto error;

	if (begin >= end)
		goto error;

	/* no merry-go-rounds allowed here */
	if (((end - begin) / sizeof(unsigned long)) < wpc)
		goto error;

	/* resize array if needed */
	if (_scrub.cnt == _scrub.sz) {
		_scrub.sec = krealloc(_scrub.sec, (_scrub.sz + SCRUB_REALLOC) *
				      sizeof(struct memscrub_sec));

		bzero(&_scrub.sec[_scrub.sz], sizeof(struct memscrub_sec) * SCRUB_REALLOC);
		_scrub.sz += SCRUB_REALLOC;
	}

	_scrub.sec[_scrub.cnt].begin = begin;
	_scrub.sec[_scrub.cnt].end   = end;
	_scrub.sec[_scrub.cnt].pos   = begin;
	_scrub.sec[_scrub.cnt].wpc   = wpc;
	_scrub.cnt++;

	return 0;
error:
	return -EINVAL;
}
EXPORT_SYMBOL(memscrub_seg_add);


/**
 * @brief remove a memory segment from scrubber
 *
 * @param being	the start address
 * @param end   the end address
 *
 * @returns 0 on success, otherwise error
 */

int memscrub_seg_rem(unsigned long begin, unsigned long end)
{
	size_t i;


	for (i = 0; i < _scrub.sz; i++) {
		if (begin == _scrub.sec[i].begin)
			if (end == _scrub.sec[i].end)
				break;
	}

	if (i == _scrub.sz)
		return -ENOENT;


	_scrub.sec[i].begin = 0;
	_scrub.sec[i].end   = 0;
	_scrub.sec[i].pos   = 0;
	_scrub.sec[i].wpc   = 0;
	_scrub.cnt--;

	/* we found an entry to delete, now move everything
	 * after back by one field if possible
	 */
	for (i++; i < _scrub.sz; i++) {
		_scrub.sec[i].begin = _scrub.sec[i - 1].begin;
		_scrub.sec[i].end   = _scrub.sec[i - 1].end;
		_scrub.sec[i].pos   = _scrub.sec[i - 1].pos;
		_scrub.sec[i].wpc   = _scrub.sec[i - 1].wpc;
	}


	return 0;
}
EXPORT_SYMBOL(memscrub_seg_rem);




int memscrub_init(void)
{
	struct task_struct *t;

	t = kthread_create(mem_do_scrub, NULL, 0, "SCRUB");
	BUG_ON(!t);

	/* run for at most 500 µs every 125 ms (~0.4 % CPU) */
	kthread_set_sched_edf(t, 125 * 1000, 120*1000, 500);

	BUG_ON(kthread_wake_up(t) < 0);

	return 0;
}

device_initcall(memscrub_init);
