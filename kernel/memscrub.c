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
#include <kernel/sysctl.h>
#include <kernel/memscrub.h>


/* if we need more space, this is how many entries we will add */
#define SCRUB_REALLOC 10

struct memscrub_sec {
	unsigned long begin;
	unsigned long end;
	unsigned long pos;
	unsigned short wpc;
};
/* this is where we keep track of scrubbing sections */
static struct {
	struct memscrub_sec *sec;
	int    sz;
	int    cnt;
} _scrub;


static ssize_t memscrub_show(__attribute__((unused)) struct sysobj *sobj,
			     __attribute__((unused)) struct sobj_attribute *sattr,
			     char *buf)
{
	int i;
	size_t ret;
	size_t n = 0;


	if (!strcmp(sattr->name, "scrub_sections")) {

		for (i = 0; i < _scrub.cnt; i++) {

			ret = sprintf(buf, "0x%08lx 0x%08lx 0x%08lx\n", _scrub.sec[i].begin,
									_scrub.sec[i].end,
									_scrub.sec[i].pos);
			buf += ret;
			n   += ret;
		}
	}

	return n;
}


static ssize_t memscrub_store(__attribute__((unused)) struct sysobj *sobj,
			      __attribute__((unused)) struct sobj_attribute *sattr,
			      __attribute__((unused)) const char *buf,
			      __attribute__((unused)) size_t len)
{
	uint32_t begin;
	uint32_t end;
	uint16_t wpc;

	char *p;


	p = strtok((char *) buf, " ");
	if (!p)
		return -1;
	begin = strtol(p, NULL, 0);

	p = strtok(NULL, " ");
	if (!p)
		return -1;
	end = strtol(p, NULL, 0);

	p = strtok(NULL, " ");
	if (!p)
		return -1;
	wpc = strtol(p, NULL, 0);

	if (!strcmp("section_add", sattr->name))
		return memscrub_seg_add(begin, end, wpc);

	if (!strcmp("section_rem", sattr->name))
		return memscrub_seg_rem(begin, end);

	return 0;
}






__extension__
static struct sobj_attribute scrub_sections_attr = __ATTR(scrub_sections,
							  memscrub_show,
							  NULL);
__extension__
static struct sobj_attribute section_add_attr = __ATTR(section_add,
						       NULL,
						       memscrub_store);
__extension__
static struct sobj_attribute section_remove_attr = __ATTR(section_rem,
							  NULL,
							  memscrub_store);
__extension__
static struct sobj_attribute *memscrub_attributes[] = {&scrub_sections_attr,
						       &section_add_attr,
						       &section_remove_attr,
						       NULL};


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

		for (i = 0; i < _scrub.cnt; i++) {

			begin = _scrub.sec[i].begin;
			end   = _scrub.sec[i].end;
			pos   = _scrub.sec[i].pos;


			n = (end - pos) / sizeof(unsigned long);

			if (n < _scrub.sec[i].wpc) {
				pos = memscrub(pos, n);
				n = _scrub.sec[i].wpc - n;
				_scrub.sec[i].pos = pos = begin;

			} else {
				n = _scrub.sec[i].wpc;
			}

			if (pos + n < end)
				_scrub.sec[i].pos = memscrub(pos, n);
		}

		/* do not yield if remaining RT > 1/8 of the allocated WCET,
		 * otherwise continue; this prevents slowdowns if
		 * a full scrub cycle of all sections did not complete
		 * within one scheduled period
		 */
		sched_maybe_yield(8);
	}

	return 0;
}


/**
 * @brief add a memory segment to scrub
 *
 * @param being	the start address
 * @param end   the end address (uppper bound, not included in scrub)
 * @param len	the number of words to scrub per scrubbing cycle
 *
 * @note the addresses must be word-aligned
 *
 * @returns 0 on success, otherwise error
 */

int memscrub_seg_add(unsigned long begin, unsigned long end, unsigned short wpc)
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
	_scrub.sec[_scrub.cnt].end   = end - sizeof(unsigned long);
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
 * @param end   the end address (upper bound, not included)
 *
 * @returns 0 on success, otherwise error
 */

int memscrub_seg_rem(unsigned long begin, unsigned long end)
{
	size_t i;


	for (i = 0; i < _scrub.sz; i++) {
		if (begin == _scrub.sec[i].begin)
			if (end == _scrub.sec[i].end + sizeof(unsigned long))
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

	/* XXX  we explicitly set cpu 0 affinity and runtime
	 * here, but actually we want that to be configurable
	 * so it can be set on a per-mission basis
	 */

	t = kthread_create(mem_do_scrub, NULL, 0, "SCRUB");
	BUG_ON(!t);

	/* run for at most 2 ms every 125 ms (~1.6 % CPU) */
	kthread_set_sched_edf(t, 125 * 1000, 120*1000, 2 * 1000);

	BUG_ON(kthread_wake_up(t) < 0);

	return 0;
}

device_initcall(memscrub_init);



/**
 * @brief initialise the sysctl entries for the memory scrubber
 *
 * @return -1 on error, 0 otherwise
 *
 * @note we set this up as a late initcall since we need sysctl to be
 *	 configured first
 */

static int memscrub_init_sysctl(void)
{
	struct sysobj *sobj;


	sobj = sysobj_create();

	if (!sobj)
		return -1;

	sobj->sattr = memscrub_attributes;

	sysobj_add(sobj, NULL, sysctl_root(), "memscrub");

	return 0;
}
late_initcall(memscrub_init_sysctl);

