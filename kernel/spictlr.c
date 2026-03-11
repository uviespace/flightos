/**
 * @file kernel/spictlr.c
 *
 * @brief SPI ctlr abstraction
 *
 * XXX WARNING this implementation is very preliminary and only tested for
 * use with single chips and single buses 
 */

#include <list.h>
#include <kernel/kmem.h>
#include <kernel/spictlr.h>
#include <kernel/kernel.h>
#include <string.h>


static LIST_HEAD(spi_ctlr_list);


void spi_msg_init(struct spi_msg *msg)
{
	memset(msg, 0, sizeof(*msg));

	INIT_LIST_HEAD(&msg->transfers);
}


static void spi_msg_add_tail(struct spi_transfer *t, struct spi_msg *m)
{
	list_add_tail(&t->transfer_list, &m->transfers);
}


static void spi_msg_init_with_transfers(struct spi_msg *m,
					struct spi_transfer *xfers,
					unsigned int num_xfers)
{
	size_t i;

	spi_msg_init(m);

	for (i = 0; i < num_xfers; ++i)
		spi_msg_add_tail(&xfers[i], m);
}


static int spi_transfer(struct spi_dev *spi, struct spi_msg *msg)
{
	struct spi_transfer *xfer;


	xfer = list_first_entry(&msg->transfers, struct spi_transfer, transfer_list);

	list_for_each_entry(xfer, &msg->transfers, transfer_list) {

		if (!xfer->len)
			continue;

		if (xfer->tx_buf || xfer->rx_buf)
			 spi->ctlr->transfer_one(spi->ctlr, msg->spi, xfer);
	}


	return 0;
}


int spi_sync(struct spi_dev *spi, struct spi_msg *msg)
{
	int status;
	struct spi_ctlr *ctlr = spi->ctlr;


	spin_lock_raw(&ctlr->bus_lock_spinlock);
	status = spi_transfer(spi, msg);
	spin_unlock(&ctlr->bus_lock_spinlock);

	if (!status)
		status = msg->status;

	return status;
}


int spi_sync_transfer(struct spi_dev *spi,
		      struct spi_transfer *xfers,
		      unsigned int num_xfers)
{
	struct spi_msg msg;


	spi_msg_init_with_transfers(&msg, xfers, num_xfers);
	msg.spi = spi;

	return spi_sync(spi, &msg);
}


struct spi_dev *spi_new_dev(struct spi_ctlr *ctlr, struct spi_board_info *chip)
{
	struct spi_dev *dev;


	if (!ctlr)
		return NULL;

	dev = kzalloc(sizeof(*dev));
	if (!dev)
		return NULL;

	dev->max_speed_hz = chip->max_speed_hz;
	dev->mode = chip->mode;
	dev->irq  = chip->irq;
	dev->ctlr = ctlr;
	dev->chip_select = chip->chip_select;

	ctlr->setup(dev);

	return dev;
}


void spi_release_dev(struct spi_dev *dev)
{
	kfree(dev->ctlr_data);
	kfree(dev);
}


int spi_register_ctlr(struct spi_ctlr *ctlr)
{
	list_add_tail(&ctlr->node, &spi_ctlr_list);

	return 0;
}


struct spi_ctlr *spi_get_master(uint8_t bus)
{
	struct spi_ctlr *p_elem;
	struct spi_ctlr *ctlr = NULL;


	if (list_empty(&spi_ctlr_list))
	    	goto exit;

	list_for_each_entry(p_elem, &spi_ctlr_list, node) {

		if (p_elem->bus_num != bus)
			continue;

		ctlr = p_elem;
		break;
	}


exit:
	return ctlr;
}
