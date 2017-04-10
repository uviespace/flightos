/**
 * @file dsp/xentium/include/xen.h
 */

#ifndef _DSP_XENTIUM_XEN_H_
#define _DSP_XENTIUM_XEN_H_

#include <stddef.h>

#include <kernel/xentium_io.h>
#include <kernel/xentium_dev.h>


/**
 *  @bug there appears to be some kind of bug when accessing (AHB?) SDRAM
 *	 addresses from the Xentiums directly. Looks like it's using the wrong
 *	 memory content or suffering from some kind of caching effect (or timing
 *	 issues with it's NoC network bridge/DMA interaction).
 *	 Just twiddling in a loop seems to work around these issues.
 *
 *  grep for XBUG to see such spots
 */
#define XBUG()	do {							\
		volatile int itsatrap;					\
		for (itsatrap = 0; itsatrap < 10000; itsatrap++);	\
		} while(0);


void xen_signal_host(void);

void xen_set_mail(size_t mbox, unsigned long msg);

unsigned long xen_get_mail(size_t mbox);

struct xen_msg_data *xen_wait_cmd(void);
void xen_send_msg(struct xen_msg_data *m);

void xen_wait_timer(int timer, unsigned long cycles);

void xen_wait_dma(void);

void *xen_get_base_addr(size_t xen_id);


#endif /* _DSP_XENTIUM_XEN_H_ */
