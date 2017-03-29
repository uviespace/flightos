/**
 * @file dsp/xentium/lib/xen.c
 */

#include <xen.h>
#include <xentium_intrinsics.h>


/**
 * @brief raise an irq to signal the host
 */

void xen_signal_host()
{
	xen_dev_local->msg_irq = 1;
}


/**
 * @brief set the contents of a mailbox
 *
 * @param mbox the mailbox to set
 * @param msg the message to set
 */

void xen_set_mail(size_t mbox, unsigned long msg)
{
	if (mbox >= XEN_MAILBOXES)
		return;
		
	xen_dev_local->mbox[mbox] = msg;
}


/**
 * @brief retrieve the contents of a mailbox
 *
 * @returns the contents of the mailbox or always 0 if the mailbox is invalid
 */

unsigned long xen_get_mail(size_t mbox)
{
	if (mbox < XEN_MAILBOXES)
		return xen_dev_local->mbox[mbox];
	
	return 0;
}


/**
 * @brief wait for host to pass a data message for processing
 *
 * @returns a data message pointer
 */

struct xen_msg_data *xen_wait_msg(void)
{
	struct xen_msg_data *m;

	x_wait(XEN_MSG_MBOX_MASK);

	m = (struct xen_msg_data *) xen_get_mail(XEN_MSG_MBOX);

	return m;
}


/**
 * @brief wait for a timer to count to 0
 *
 * @param timer the timer to use
 * @param cycles the number of cycles to count down
 */

void xen_wait_timer(int timer, unsigned long cycles)
{
	if (timer >= XEN_TIMERS)
		return;


	xen_dev_local->timer[timer] = cycles;
	
	while(xen_dev_local->timer[timer]);
}

