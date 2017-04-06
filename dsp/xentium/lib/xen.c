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
 * @brief wait for host to pass a command data message for processing
 *
 * @returns a data message pointer
 */

struct xen_msg_data *xen_wait_cmd(void)
{
	struct xen_msg_data *m;

	x_wait(XEN_CMD_MBOX_MASK);

	m = (struct xen_msg_data *) xen_get_mail(XEN_CMD_MBOX);

	return m;
}


/**
 * @brief pass a data message and signal the host
 *
 * @param m the message to pass
 */

void xen_send_msg(struct xen_msg_data *m)
{
	xen_set_mail(XEN_MSG_MBOX, (unsigned long) m);
	xen_signal_host();
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


/**
 * @brief wait until the DMA status bit is low (i.e. transfer complete)
 * *
 */
void xen_wait_dma(void)
{
	/* XXX this doesn't seem to work sensibly, we can't x_wait() on the
	 * dma irq status bit, because it is set UNTIL we clear the
	 * dma_irq, and we'd apparently have to also clear the latter
	 * _before_ we even start the transfer. For now, it's easier to just
	 * wait on the channel status bit in noc_dma_start_transfer()
	 */
}

/**
 * @brief get the base address of a Xentium by its numeric index
 *
 * @returns the base address of the Xentium
 */

void *xen_get_base_addr(size_t idx)
{
	return (void *) (XEN_BASE_ADDR + XEN_BLOCK_SIZE * idx);
}
