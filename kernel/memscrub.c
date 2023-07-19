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
 * @defgroup memscrub Memory Scrubbing
 * @brief Implements memory scrubbing and repair of single-bit errors
 *
 * ## Overview
 *
 * This module provides functionality for memory scrubbing cycles, correctable
 * error logging and memory repair.
 * Note that only correctable errors are recorded and repaired, uncorrectable
 * errors are reported via the @ref edac module
 *
 * ## Mode of Operation
 *
 * During each activation, starting from a specified address, a number of memory
 * locations are read by forced data cache bypass in CPU-word size
 * increments. After each read, the EDC status is checked for single
 * bit errors. If an error was raised and the registered failing address
 * corresponds to the read address, it is logged for deferred memory repair.
 *
 * @startuml {memory_scrubbing.svg} "Memory Scrubbing process" width = 10
 *
 * start
 *
 * repeat
 *	: read memory;
 *	: inspect AHB status;
 *	if (SBIT error) then (yes)
 *		if (address matches) then (yes)
 *			: log error;
 *		endif
 *	endif
 * repeat while (done?)
 *
 * : mask interrrupts;
 *
 * while (logged error)
 *	: read memory;
 *	: write memory;
 * endwhile
 *
 * : unmask interrrupts;
 *
 * stop
 *
 * @enduml
 *
 *
 * ## Error Handling
 *
 * None
 *
 *
 * ## Notes
 *
 * - There is no internal discrimination regarding repair of memory locations.
 *   Basically, a re-write is attempted for every single bit error detected.
 *
 * - If the memory error log is at capacity, no new errors will be added.
 *
 * XXX TODO sysctl IF to set scrub sections
 */


#include <errno.h>
#include <list.h>
#include <compiler.h>
#include <asm-generic/io.h>
#include <asm-generic/memrepair.h>

#include <kernel/init.h>
#include <kernel/edac.h>
#include <kernel/kmem.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/sysctl.h>
#include <kernel/export.h>
#include <kernel/kthread.h>

#define MEMSCRUB_LOG_ENTRIES	10

struct memscrub_log_entry {
	uint32_t fail_addr;
	struct list_head node;
};

static struct memscrub_log_entry memscrub_log_entries[MEMSCRUB_LOG_ENTRIES];

static struct list_head	memscrub_log_used;
static struct list_head memscrub_log_free;



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
 * @brief add an entry to the error log
 *
 * @param fail_addr the address of the single-bit error
 *
 * @note If no more free slots are available, the request will be ignored and
 *       the address will not be registered until another pass is made
 */

static void memscrub_add_log_entry(uint32_t fail_addr)
{
	struct memscrub_log_entry *p_elem;


	if (list_filled(&memscrub_log_free)) {

		p_elem = list_entry((&memscrub_log_free)->next,
				    struct memscrub_log_entry, node);

		list_move_tail(&p_elem->node, &memscrub_log_used);
		p_elem->fail_addr = fail_addr;
	}
}


/**
 * @brief check the EDAC for new single bit errors
 *
 * @return  0 if no error,
 *	    1 if error,
 *	   -1 if error address does not match reference
 */

static int32_t memscrub_inspect_edac_status(uint32_t addr)
{
	uint32_t fail_addr;


	if (edac_error_detected()) {

		fail_addr = edac_get_error_addr();

		edac_error_clear();

		if (addr == fail_addr)
			memscrub_add_log_entry(fail_addr);

		else
			return -1;

		return 1;
	}

	return 0;
}


/**
 * @brief retrieve number of entries in the edac error log
 *
 * @return number of registered single-bit errors in the log
 *
 * XXX TODO sysctl stats
 */
__attribute__((unused))
static uint32_t memscrub_get_num_log_entries(void)
{
	uint32_t entries = 0;

	struct memscrub_log_entry *p_elem;
	struct memscrub_log_entry *p_tmp;

	list_for_each_entry_safe(p_elem, p_tmp, &memscrub_log_used, node)
		entries++;

	return entries;
}


/**
 * @brief retrieve number of entries in the edac error log
 *
 * @return number of addresses repaired
 */

static uint32_t memscrub_repair(void)
{

	uint32_t corr = 0;

	struct memscrub_log_entry *p_elem;
	struct memscrub_log_entry *p_tmp;


	list_for_each_entry_safe(p_elem, p_tmp, &memscrub_log_used, node) {

		mem_repair((void *) p_elem->fail_addr);

		/* repairs of error will trigger and error */
		edac_error_clear();

		/* XXX kalarm() -> EDAC, LOW, p_elem->fail_addr */
		list_move_tail(&p_elem->node, &memscrub_log_free);

		corr++;
	}


	return corr;
}



/**
 * @brief memory scrubbing of a range addr[0] ... addr[n] at a time
 *
 * Identified single-bit errors are stored in log for repair at a later time.
 *
 * @param addr the address pointer to start scrubbing from
 * @param n    the number of data words to scrub on top of starting address
 *
 * @return the next unscrubbed address
 */

static unsigned long memscrub(unsigned long addr, size_t n)
{
	unsigned long stop = addr + n;


	edac_error_clear();

	for ( ; addr < stop; addr++) {
		ioread32be((void *)addr);	/* XXX not portable */
		memscrub_inspect_edac_status(addr);
	}

	memscrub_repair();

	return stop;
}



static int mem_do_scrub(void *data)
{
	size_t n;
	size_t i;

	unsigned long begin;
	unsigned long end;
	unsigned long pos;

	while (1) {

		for (i= 0; i < _scrub.sz; i++) {

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
				      sizeof(struct edac_scrub_sec *));

		bzero(&_scrub.sec[_scrub.sz], sizeof(struct edac_scrub_sec *) * SCRUB_REALLOC);
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
	uint32_t i;
	struct task_struct *t;


	INIT_LIST_HEAD(&memscrub_log_used);
	INIT_LIST_HEAD(&memscrub_log_free);

	for (i = 0; i < ARRAY_SIZE(memscrub_log_entries); i++)
		list_add_tail(&memscrub_log_entries[i].node,
			      &memscrub_log_free);



	t = kthread_create(mem_do_scrub, NULL, 0, "SCRUB");
	BUG_ON(!t);

	/* run for at most 500 µs every 125 ms (~0.4 % CPU) */
	kthread_set_sched_edf(t, 125 * 1000, 2000, 500);

	BUG_ON(kthread_wake_up(t) < 0);




	return 0;
}

device_initcall(memscrub_init);
