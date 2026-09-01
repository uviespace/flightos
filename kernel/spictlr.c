/**
 * @file kernel/spictlr.c
 *
 * @ingroup spi_driver
 * @defgroup spi_driver SPI controller abstraction
 *
 * @brief High-level synchronous SPI controller, device, message, and transfer
 *        interfaces.
 *
 * The core keeps a list of registered controllers, looks them up by bus
 * number, creates device objects from board information, and serializes a
 * message through the controller bus lock. Each transfer is delegated to the
 * controller callbacks. The current implementation is preliminary and was
 * tested only for single chips and single buses. The declared asynchronous and
 * standalone setup interfaces have no implementation in this file; intended
 * behavior needs review.
 *
 * @startuml FlightOS SPI Subsystem
 * title SPI Subsystem Dependencies
 *
 * !pragma layout ortho
 *
 * skinparam component {
 *   BackgroundColor #E3F2FD
 *   BorderColor #333333
 * }
 * skinparam arrowColor #333333
 *
 * package "Device Drivers" {
 *   [ad7814_spi.c\n(Temperature Sensor)] as AD7814
 *   [adc128s102_spi.c\n(ADC)] as ADC128
 * }
 *
 * package "SPI Core (kernel/)" {
 *   [spictlr.c] as SPI_CORE
 *   [grspi.c\n(GRSPI Driver)] as GRSPI
 * }
 *
 * package "Hardware" {
 *   [GRSPI Hardware] as HW
 * }
 *
 * package "Abstraction" {
 *   [struct spi_ctlr] as SPI_CTLR
 *   [struct spi_dev] as SPI_DEV
 * }
 *
 * AD7814 -down-> SPI_CORE : spi_sync()\nspi_msg_init()
 * ADC128 -down-> SPI_CORE : spi_sync_transfer()\nspi_new_dev()
 *
 * SPI_CORE -down-> SPI_CTLR : generic API
 * GRSPI -up-> SPI_CTLR : implements
 * GRSPI -down-> HW : register I/O
 *
 * AD7814 -down-> SPI_DEV : spi_dev
 * ADC128 -down-> SPI_DEV : spi_dev
 *
 * note bottom of SPI_CTLR
 *   spi_ctlr struct:
 *   setup()
 *   transfer_one()
 * end note
 *
 * note bottom of SPI_DEV
 *   spi_dev struct:
 *   chip_select()
 * end note
 *
 * @enduml
 */

#include <list.h>
#include <kernel/kmem.h>
#include <kernel/spictlr.h>
#include <kernel/kernel.h>
#include <string.h>


static LIST_HEAD(spi_ctlr_list);


/**
 * @brief initialise an SPI message
 *
 * @param msg a pointer to the SPI message to initialise
 */

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


/**
 * @brief perform a synchronous SPI transfer
 *
 * @param spi a pointer to the SPI device
 * @param msg a pointer to the SPI message to transfer
 *
 * @return 0 on success, negative error code on failure
 */

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


/**
 * @brief perform a synchronous SPI transfer from an array of transfers
 *
 * @param spi a pointer to the SPI device
 * @param xfers a pointer to an array of SPI transfers
 * @param num_xfers the number of transfers in the array
 *
 * @return 0 on success, negative error code on failure
 */

int spi_sync_transfer(struct spi_dev *spi,
		      struct spi_transfer *xfers,
		      unsigned int num_xfers)
{
	struct spi_msg msg;


	spi_msg_init_with_transfers(&msg, xfers, num_xfers);
	msg.spi = spi;

	return spi_sync(spi, &msg);
}


/**
 * @brief create and register a new SPI device on a controller
 *
 * @param ctlr a pointer to the SPI controller
 * @param chip a pointer to the board info describing the SPI device
 *
 * @return a pointer to the new SPI device, or NULL on error
 */

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


/**
 * @brief release an SPI device and free its resources
 *
 * @param dev a pointer to the SPI device to release
 */

void spi_release_dev(struct spi_dev *dev)
{
	kfree(dev->ctlr_data);
	kfree(dev);
}


/**
 * @brief register an SPI controller
 *
 * @param ctlr a pointer to the SPI controller to register
 *
 * @return 0 on success
 */

int spi_register_ctlr(struct spi_ctlr *ctlr)
{
	list_add_tail(&ctlr->node, &spi_ctlr_list);

	return 0;
}


/**
 * @brief retrieve an SPI controller by bus number
 *
 * @param bus the bus number of the controller to retrieve
 *
 * @return a pointer to the SPI controller, or NULL if not found
 */

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
