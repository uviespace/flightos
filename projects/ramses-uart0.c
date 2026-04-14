/**
 * @file project/ramses-uart0.c
 *
 * @brief a driver for the RAMSES DPM UART interface to the MTC FPGA
 */

#include <kernel/init.h>
#include <kernel/types.h>
#include <kernel/export.h>
#include <kernel/kmem.h>
#include <kernel/kthread.h>
#include <kernel/printk.h>
#include <asm/leon_reg.h>
#include <asm-generic/io.h>
#include <kernel/irq.h>
#include <asm/io.h>
#include <asm/spinlock.h>


#define TX_EMPTY (1 << 2)
#define TX_FULL  (1 << 9)

#define STATUS_ERR_FE	(1 << 6)
#define STATUS_ERR_PE	(1 << 5)
#define STATUS_ERR_OV  (1 << 4)


#define CTRL_RX_IRQ		(1 << 2)
#define CTRL_TX_IRQ		(1 << 3)
#define CTRL_TX_FIFO_IRQ	(1 << 9)
#define CTRL_RX_FIFO_IRQ	(1 << 10)
#define CTRL_TX_EN		(1 << 1)



static void ramses_rs485_tx_select(bool enable)
{
	uint32_t dir;
	uint32_t out;

	struct leon3_grgpio_registermap *reg = (void *)LEON3_BASE_ADDRESS_GRGPIO_1;


	dir = ioread32be(&reg->ioport_direction);
	out = ioread32be(&reg->ioport_output_value);

	if (enable) {
		iowrite32be(dir |  (1 << 5), &reg->ioport_direction);
		iowrite32be(out |  (1 << 5), &reg->ioport_output_value);
	} else {
		iowrite32be(out & ~(1 << 5), &reg->ioport_output_value);
		iowrite32be(dir |  (1 << 5), &reg->ioport_direction);
	}
}


static int rs485_write_internal(void *buf, size_t nbyte)
{
	size_t cnt = 0;
	char *c = buf;
	uint32_t status;
	uint32_t ctrl;

	struct leon3_apbuart_registermap *uart = (void *)LEON3_BASE_ADDRESS_APBUART;


	/* if we see a link-level error, to re-sync, we
	 * attempt to empty the RX fifo of stale bytes before TXing the
	 * next command
	 */
	while ((ioread32be(&uart->status) >> 26))
		ioread32be(&uart->data);

	status = ioread32be(&uart->status);	/* clear all errors */
	if (status & (STATUS_ERR_FE | STATUS_ERR_PE | STATUS_ERR_OV)) {

		status &= ~(STATUS_ERR_FE | STATUS_ERR_PE | STATUS_ERR_OV);
		iowrite32be(status, &uart->status);
	}

	while (nbyte) {
		uart->data = c[cnt] & 0xff;
		cnt++;
		nbyte--;
	}

	/* switch transceiver back to tx */
	ramses_rs485_tx_select(1);
	/* enable IRQs, TX */
	ctrl = ioread32be(&uart->ctrl);
	ctrl |= CTRL_RX_IRQ | CTRL_TX_IRQ | CTRL_TX_EN;
	iowrite32be(ctrl, &uart->ctrl);


	return cnt;
}


int rs485_write(void *buf, size_t nbyte)
{
	rs485_write_internal(buf, nbyte);

	return 0;
}


static siginfo_t info;

static irqreturn_t rs485_irq(unsigned int irq, void *userdata)
{
	struct leon3_apbuart_registermap *uart = (void *)LEON3_BASE_ADDRESS_APBUART;


	/* busy-wait until the last bit has been shifted out by the core
	 * this takes about 100 µs from the IRQ
	 */
	while (!(ioread32be(&uart->status) & 0x2));

	/* tx done, switch external transceiver to RX */
	ramses_rs485_tx_select(0);

	if ((ioread32be(&uart->status) >> 26) > 3) {

		uint8_t *p = (void *)&info;
		uint32_t status;
		uint32_t ctrl;


		/* we always expect a 4-byte response */
		p[0] = ioread32be(&uart->data) & 0xff;
		p[1] = ioread32be(&uart->data) & 0xff;
		p[2] = ioread32be(&uart->data) & 0xff;
		p[3] = ioread32be(&uart->data) & 0xff;

		/* disable IRQs, TX enable bit
		 * NOTE: we may have encountered an unknown erratum here;
		 * with the FPGA we see an apparent communication error for
		 * ~1% of commands, with a rate of ~800/s
		 * An oscilloscope trace shows that this is indeed a
		 * transmission problem, as the TX_EN line controlling the
		 * transceiver goes high in rs485_write_internal(), but
		 * no data is being clocked out.
		 *
		 * I therefore had the idea to simply toggle the transmit bit
		 * off at some point (i.e. here) and only back on when we have
		 * queued up our 6 command bytes in the TX FIFO
		 * voila...no more missed messaged
		 */
		ctrl = ioread32be(&uart->ctrl);
		ctrl &= ~(CTRL_RX_IRQ | CTRL_TX_IRQ | CTRL_TX_EN);
		iowrite32be(ctrl, &uart->ctrl);

		status = ioread32be(&uart->status);
		if (status & (STATUS_ERR_FE | STATUS_ERR_PE | STATUS_ERR_OV)) {

			/* empty fifo if any of these occured */
			while ((ioread32be(&uart->status) >> 26))
				ioread32be(&uart->data);

			status = ioread32be(&uart->status);
			status &= ~(STATUS_ERR_FE | STATUS_ERR_PE | STATUS_ERR_OV);
			iowrite32be(status, &uart->status);

			goto exit;
		}

		/* note: we abuse info to transport our 4-byte response to
		 * userspace; this saves us another syscall
		 */
		ksignal_send_info(111, &info);
	}

exit:
	return IRQ_HANDLED;
}

/**
 * @brief initalises rs485
 */


int rs485_init(void)
{
	int ret;

	struct leon3_apbuart_registermap *uart = (void *)LEON3_BASE_ADDRESS_APBUART;

	ramses_rs485_tx_select(0);

	/* even parity, RX enable, clear everything else */
	uart->ctrl = (1 << 5) | (1 << 0);

	uart->scaler = 64;	/* closest scaler for 115200 baud target @60MHz sysclk */


	irq_set_affinity(2, 1); /* we handle the UART on the second CPU */
	ret = irq_request(2, ISR_PRIORITY_NOW, rs485_irq, NULL);
	if (ret)
		return -1;

	return 0;
}
late_initcall(rs485_init);

