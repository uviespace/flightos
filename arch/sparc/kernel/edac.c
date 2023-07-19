/**
 * @file   edac.c
 * @ingroup edac
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 * @date   February, 2016
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
 * @defgroup edac EDAC error handling
 * @brief Implements EDAC error handling and utility functions
 *
 *
 * ## Overview
 *
 * This module implements handling of double-bit EDAC errors raised either via
 * SPARC v8 data access exception trap 0x9 or the AHB STATUS IRQ 0x1.
 *
 * ## Mode of Operation
 *
 * The @ref traps module is needed to install the custom data access exception
 * trap handler, the AHB irq handler must be installed via @ref irq_dispatch.
 *
 * If an error is detected in normal memory, its location is reported. If the
 * source of the double bit error is the @ref iwf_flash, the last flash read
 * address is identified and reported.
 *
 *
 * @startuml {edac_handler.svg} "EDAC trap handler" width=10
 * :IRQ/trap;
 * if (EDAC) then (yes)
 *	-[#blue]->
 *	if (correctable) then (yes)
 *		:read address;
 *		if (FLASH) then (yes)
 *			:get logical address;
 *		endif
 *		: report address;
 *	endif
 * endif
 * end
 * @enduml
 *
 *
 * ## Error Handling
 *
 * None
 *
 * ## Notes
 *
 * - This module provides EDAC checkbit generation that is used in @ref memcfg
 *   to inject single or double bit errors.
 *
 */
#include <ahb.h>
#include <traps.h>
#include <errno.h>
#include <asm/io.h>

#include <leon3_memcfg.h>
#include <kernel/edac.h>
#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/sysctl.h>
#include <kernel/irq.h>
#include <kernel/kmem.h>
#include <kernel/init.h>

/* XXX meh... need central definition of these */
#define GR712_IRL1_AHBSTAT	1


/* if we need more space, this is how many entries we will add */
#define CRIT_REALLOC 10

struct edac_crit_sec {
	void *begin;
	void *end;
};
/* this is where we keep track of critical sections */
static struct {
	struct edac_crit_sec *sec;
	int    sz;
	int    cnt;
} _crit;




/**
 * the reset function to call when a critical error is encountered
 */

static void (*do_reset)(void *data);

/**
 * the user data to supply to the reset function
 */

static void *reset_data;



static struct {
	uint32_t edac_single;
	uint32_t edac_double;
	uint32_t edac_last_single_addr;
	uint32_t edac_last_double_addr;
} edacstat;

#if (__sparc__)
#define UINT32_T_FORMAT		"%lu"
#else
#define UINT32_T_FORMAT		"%u"
#endif

static ssize_t edac_show(__attribute__((unused)) struct sysobj *sobj,
			 __attribute__((unused)) struct sobj_attribute *sattr,
			 char *buf)
{
	if (!strcmp(sattr->name, "singlefaults"))
		return sprintf(buf, UINT32_T_FORMAT, edacstat.edac_single);

	if (!strcmp(sattr->name, "doublefaults"))
		return sprintf(buf, UINT32_T_FORMAT, edacstat.edac_double);

	if (!strcmp(sattr->name, "lastsingleaddr"))
		return sprintf(buf, UINT32_T_FORMAT,
			       edacstat.edac_last_single_addr);

	if (!strcmp(sattr->name, "lastdoubleaddr"))
		return sprintf(buf, UINT32_T_FORMAT,
			       edacstat.edac_last_double_addr);



	return 0;
}


__extension__
static struct sobj_attribute singlefaults_attr = __ATTR(singlefaults,
							edac_show,
							NULL);
__extension__
static struct sobj_attribute doublefaults_attr = __ATTR(doublefaults,
							edac_show,
							NULL);
__extension__
static struct sobj_attribute lastsingleaddr_attr = __ATTR(lastsingleaddr,
							  edac_show,
							  NULL);
__extension__
static struct sobj_attribute lastdoubleaddr_attr = __ATTR(lastdoubleaddr,
							  edac_show,
							  NULL);
__extension__
static struct sobj_attribute *edac_attributes[] = {&singlefaults_attr,
						   &doublefaults_attr,
						   &doublefaults_attr,
						   &lastsingleaddr_attr,
						   &lastdoubleaddr_attr,
						   NULL};


/**
 * @brief generate an BCH EDAC checkbit
 *
 * @param value the 32 bit value to generate the checkbit for
 * @param bits an array of bit indices to XOR
 * @param len the lenght of the bit index array
 *
 * @return a checkbit
 *
 * @note see GR712-UM v2.3 p. 60
 */

static uint8_t edac_checkbit_xor(const uint32_t value,
				 const uint32_t *bits,
				 const uint32_t len)
{
	uint8_t val = 0;

	uint32_t i;


	for (i = 0; i < len; i++)
		val ^= (value >> bits[i]);

	return (val & 0x1);
}


/**
 * @brief generate BCH EDAC checkbits
 *
 * @param value the 32 bit value to generate the checkbits for
 *
 * @return bch the checkbits
 *
 * @note see GR712-UM v2.3 p. 60
 *
 */

static uint8_t bch_edac_checkbits(const uint32_t value)
{
	uint8_t bch = 0;

	static const uint32_t CB[8][16] = {
		{0, 4,  6,  7,  8,  9, 11, 14, 17, 18, 19, 21, 26, 28, 29, 31},
		{0, 1,  2,  4,  6,  8, 10, 12, 16, 17, 18, 20, 22, 24, 26, 28},
		{0, 3,  4,  7,  9, 10, 13, 15, 16, 19, 20, 23, 25, 26, 29, 31},
		{0, 1,  5,  6,  7, 11, 12, 13, 16, 17, 21, 22, 23, 27, 28, 29},
		{2, 3,  4,  5,  6,  7, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31},
		{8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31},
		{0, 1,  2,  3,  4,  5,  6, 7,  24, 25, 26, 27, 28, 29, 30, 31},
	};


	bch |=   edac_checkbit_xor(value, &CB[0][0], 16)        << 0;
	bch |=   edac_checkbit_xor(value, &CB[1][0], 16)        << 1;
	bch |= (~edac_checkbit_xor(value, &CB[2][0], 16) & 0x1) << 2;
	bch |= (~edac_checkbit_xor(value, &CB[3][0], 16) & 0x1) << 3;
	bch |=   edac_checkbit_xor(value, &CB[4][0], 16)	<< 4;
	bch |=   edac_checkbit_xor(value, &CB[5][0], 16)	<< 5;
	bch |=   edac_checkbit_xor(value, &CB[6][0], 16)        << 6;
	bch |=   edac_checkbit_xor(value, &CB[7][0], 16)        << 7;


	return bch & 0x7f;
}



/**
 * @brief check if the failing address is in a critical segment
 *
 * @param addr the address to check
 *
 * @return 0 if not critical, not null otherwise
 *
 * @note critical sections typically are: CPU stack space, SW image in RAM
 */

static uint32_t edac_error_in_critical_section(void *addr)
{
	size_t i;


	for (i = 0; i < _crit.sz; i++) {
		if (addr >= _crit.sec[i].begin)
			if (addr <= _crit.sec[i].end)
				return 1;
	}

	return 0;
}



/**
 * @brief investigate AHB status for source of error
 *
 * @return 1 if source was edac error, 0 otherwise
 *
 * @note turns out the AHB irq is raised on single bit errors after all;
 *       sections 5.10.3 and 7 in the GR712-UM docs are a bit ambiguous
 *       regarding that; since we should not correct errors while the other
 *       CPU is running, we'll continue to use the scrubbing mechanism as before
 */

static uint32_t edac_error(void)
{
	static uint32_t addr;


	/* was not EDAC related, ignore */
	if (!ahbstat_new_error())
		return 0;


	addr = ahbstat_get_failing_addr();

	ahbstat_clear_new_error();

	/* ignore correctable errors, but update statistics */
	if (ahbstat_correctable_error()) {
		edacstat.edac_single++;
		edacstat.edac_last_single_addr = addr;
		return 0;
	}


	edacstat.edac_double++;
	edacstat.edac_last_double_addr = addr;

	switch (addr) {
	default:
		/* XXX kalarm() */
		if (edac_error_in_critical_section((void *)addr))
			if (do_reset)
				do_reset(reset_data);
		/* otherwise overwrite with all bits set */
		iowrite32be(-1, (void *) addr);
		break;
	}


	return 1;
}

/**
 * set up:
 *
 *	trap_handler_install(0x9, data_access_exception_trap);
 *	register edac trap to ahb irq
 *
 *	for testing with grmon:
 *	dsu_clear_cpu_break_on_trap(0);
 *	dsu_clear_cpu_break_on_error_trap(0);
 *
 * @note this is not triggered for single faults in RAM (needs manual checking)
 *       but still raised by the FPGA if the FLASH error is correctable
 *
 *       XXX this is old cheops stuff; I'm unsure if this can occur in
 *	situations where double faults are encountered TODO: test
 *
 */
__attribute__((unused))
static void edac_trap(void)
{
	edac_error();
}


/**
 * @brief AHB irq handler for EDAC interrupts
 */

static irqreturn_t edac_ahb_irq(unsigned int irq, void *userdata)
{
	edac_error();

	return 0;
}


/**
 * @brief set the reset function to call on critical error
 *
 * @param handler a reset callback function
 * @param data a pointer to arbitrary data supplied to the callback function
 */

static void set_reset_handler(void (*handler)(void *data),
		       void *data)
{
	do_reset = handler;
	reset_data = data;
}


/**
 * @brief initialise the sysctl entries for the edac module
 *
 * @return -1 on error, 0 otherwise
 *
 * @note we set this up as a late initcall since we need sysctl to be
 *	 configured first
 */

static int edac_init_sysctl(void)
{
	struct sysobj *sobj;


	sobj = sysobj_create();

	if (!sobj)
		return -1;

	sobj->sattr = edac_attributes;

	sysobj_add(sobj, NULL, sysctl_root(), "edac");

	return 0;
}
late_initcall(edac_init_sysctl);


static int crit_seg_add(void *begin, void *end)
{
	/* resize array if needed */
	if (_crit.cnt == _crit.sz) {
		_crit.sec = krealloc(_crit.sec, (_crit.sz + CRIT_REALLOC) *
				     sizeof(struct edac_crit_sec *));

		bzero(&_crit.sec[_crit.sz], sizeof(struct edac_crit_sec *) * CRIT_REALLOC);
		_crit.sz += CRIT_REALLOC;
	}

	_crit.sec[_crit.cnt].begin = begin;
	_crit.sec[_crit.cnt].end   = end;
	_crit.cnt++;

	return 0;
}


static int crit_seg_rem(void *begin, void *end)
{
	size_t i;


	for (i = 0; i < _crit.sz; i++) {
		if (begin == _crit.sec[i].begin)
			if (end == _crit.sec[i].end)
				break;
	}

	if (i == _crit.sz)
		return -ENOENT;


	_crit.sec[i].begin = NULL;
	_crit.sec[i].end   = NULL;
	_crit.cnt--;

	/* we found an entry to delete, now move everything
	 * after back by one field if possible
	 */
	for (i++; i < _crit.sz; i++) {
		_crit.sec[i].begin = _crit.sec[i - 1].begin;
		_crit.sec[i].end   = _crit.sec[i - 1].end;
	}

	return 0;
}

static int error_detected(void)
{
	return ahbstat_new_error();
}

static unsigned long get_error_addr(void)
{
	return ahbstat_get_failing_addr();
}

static void error_clear(void)
{
	ahbstat_clear_new_error();
}

static void enable_edac(void)
{
#ifdef CONFIG_LEON3
	leon3_memcfg_enable_ram_edac();
#endif /* CONFIG_LEON3 */
}


static void disable_edac(void)
{
#ifdef CONFIG_LEON3
	leon3_memcfg_disable_ram_edac();
#endif /* CONFIG_LEON3 */
}


static void inject_fault(void *addr, uint32_t mem_value, uint32_t edac_value)
{
#ifdef CONFIG_LEON3
	leon3_memcfg_bypass_write(addr, mem_value, bch_edac_checkbits(edac_value));
#endif /* CONFIG_LEON3 */
}


/**
 * the configuration for the high-level edac manager
 */

static struct edac_dev leon_edac = {
	.enable		   = enable_edac,
	.disable	   = disable_edac,
        .crit_seg_add	   = crit_seg_add,
        .crit_seg_rem	   = crit_seg_rem,
	.error_detected	   = error_detected,
	.get_error_addr	   = get_error_addr,
	.error_clear	   = error_clear,
	.inject_fault	   = inject_fault,
        .set_reset_handler = set_reset_handler,
};


/**
 * @brief initialise the EDAC subsystem on the LEON
 */

void leon_edac_init(void)
{



	edac_init(&leon_edac);

	/* EDAC double faults, could need extra handler in
	 * arch/sparc/kernel/data_access_exception.c
	*/
	trap_handler_install(0x9, data_access_exception_trap);
#if 0
	/* XXX this is old cheops stuff, keeping it here to check later */
	trap_handler_install(0x2, reset_trap);
#endif


#ifdef CONFIG_LEON3

	irq_request(GR712_IRL1_AHBSTAT, ISR_PRIORITY_NOW, edac_ahb_irq, NULL);
#endif /* CONFIG_LEON3 */
}

