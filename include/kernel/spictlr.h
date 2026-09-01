/**
 * @file   kernel/spictlr.h
 * @ingroup spi_driver
 *
 * @brief High-level SPI controller, device, message, and transfer contracts.
 *
 * The declarations define the kernel-side boundary implemented by
 * `kernel/spictlr.c`; hardware-specific register access and transfer behavior
 * are supplied by a controller callback such as GRSPI.
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   2026
 *
 * @note this roughly follows the Linux SPI API, please look at the
 *	 corresponding documentation for more information on how this works
 *
 * XXX WARNING this implementation is very preliminary and only tested for
 * use with single chips and single buses 
 */

#ifndef SPCTLR_H
#define SPCTLR_H

#include <list.h>
#include <kernel/types.h>
#include <asm/spinlock.h>



#define	SPI_CPHA		(1 << 0)	/*!< clock phase */
#define	SPI_CPOL		(1 << 1)	/*!< clock polarity */
#define	SPI_MSB_FIRST		(1 << 2)	/*!< bit clock out order */
#define	SPI_LOOP		(1 << 3)	/*!< test loopback */

/* the standard modes */
/** @brief idle polarity low, sample on rising edge, shift out on falling edge */
#define	SPI_MODE_0		(0 | 0)
/** @brief idle polarity low, sample on falling edge, shift out on rising edge */
#define	SPI_MODE_1		(0 | SPI_CPHA)
/** @brief idle polarity high, sample on falling edge, shift out on rising edge */
#define	SPI_MODE_2		(SPI_CPOL | 0)
/** @brief idle polarity high, sample on rising edge, shift out on falling edge */
#define	SPI_MODE_3		(SPI_CPOL | SPI_CPHA)



/**
 * @brief an SPI message containing a list of transfers
 */
struct spi_msg {
	struct spi_dev		*spi;		/*!< target SPI device */
	struct list_head        transfers;	/*!< list of spi_transfer */
	int			status;		/*!< transfer status */
	void			*userdata;	/*!< caller-provided user data */
	void (*complete)(void *userdata);	/*!< completion callback */
};

/**
 * @brief a single SPI transfer
 */
struct spi_transfer {
	const void	*tx_buf;	/*!< transmit buffer (or NULL) */
	void		*rx_buf;	/*!< receive buffer (or NULL) */
	uint32_t	len;		/*!< transfer length in bytes */
	uint32_t	speed_hz;	/*!< transfer speed in Hz */
	uint8_t		bits_per_word;	/*!< bits per word */

	struct list_head transfer_list;	/*!< node in the message's transfer list */
};


/**
 * @brief SPI controller
 */
struct spi_ctlr {

	void	*ctlr_dev;		/*!< controller device data */
	uint8_t	 bus_num;		/*!< controller bus number */

	int (*setup)(struct spi_dev *spi);	/*!< device setup callback */
	int (*transfer_one)(struct spi_ctlr *ctlr,	/*!< single transfer callback */
			    struct spi_dev *spi,
			    struct spi_transfer *transfer);

	struct spinlock bus_lock_spinlock;	/*!< bus locking */
	struct list_head node;			/*!< node in controller list */
};


/**
 * @brief SPI target device
 */
struct spi_dev {

	struct spi_ctlr	*ctlr;		/*!< controlling controller */
	void		*ctlr_data;	/*!< controller-specific data */
	uint32_t	max_speed_hz;	/*!< maximum supported speed */
	uint32_t	mode;		/*!< SPI mode (CPOL/CPHA etc.) */

	void (*chip_select)(bool enable);	/*!< chip select callback */

	uint8_t		irq;		/*!< IRQ number */
	uint8_t		bits_per_word;	/*!< bits per word */
};


/**
 * @brief SPI board-level device information
 *
 * @note we currently target only the GR712RC SPI controller, so we
 *       provide a chip select call, rather than a number for this
 *       board; if a chip select is available in an other controller
 *       add a "int num_chip_select" here and check whether chip_select
 *       is NULL to determine which mechanism to use
 */
struct spi_board_info {

	uint32_t mode;			/*!< SPI mode */
	uint32_t max_speed_hz;		/*!< maximum supported speed */

	void (*chip_select)(bool enable);	/*!< chip select callback */
	uint8_t irq;				/*!< IRQ number */
};


/**
 * @brief register an SPI controller
 * @param ctlr: the controller to register
 * @return 0 on success, negative error code on failure
 */
int spi_register_ctlr(struct spi_ctlr *ctlr);

/**
 * @brief get a registered SPI controller by bus number
 * @param bus: the bus number to look up
 * @return pointer to the controller, or NULL if not found
 */
struct spi_ctlr *spi_get_master(uint8_t bus);

/**
 * @brief create a new SPI device on a controller
 * @param ctlr: the controller to create the device on
 * @param chip: board info describing the device
 * @return pointer to the new device, or NULL on failure
 */
struct spi_dev *spi_new_dev(struct spi_ctlr *ctlr, struct spi_board_info *chip);

/**
 * @brief release an SPI device
 * @param dev: the device to release
 */
void spi_release_dev(struct spi_dev *dev);

/**
 * @brief set up an SPI device (configure mode/speed)
 * @param spi: the device to configure
 * @return 0 on success, negative error code on failure
 */
int spi_setup(struct spi_dev *spi);

/**
 * @brief perform multiple blocking transfers; the target is unlocked and the
 *        chip select is disabled between individual transfers
 * @param spi: the SPI device
 * @param xfers: array of transfers to perform
 * @param num_xfers: number of transfers in the array
 * @return 0 on success, negative error code on failure
 */
int spi_sync_transfer(struct spi_dev *spi, struct spi_transfer *xfers, unsigned int num_xfers);

/**
 * @brief perform a single non-blocking transfer; can be used from an ISR.
 *        When the transfer is finalised, the result is propagated via the
 *        complete() call in the message configuration
 * @param spi: the SPI device
 * @param msg: the message describing the transfer
 * @return 0 on success, negative error code on failure
 */
int spi_async(struct spi_dev *spi, struct spi_msg *msg);

/**
 * @brief perform a single blocking transfer
 * @param spi: the SPI device
 * @param msg: the message describing the transfer
 * @return 0 on success, negative error code on failure
 */
int spi_sync(struct spi_dev *spi, struct spi_msg *msg);

/**
 * @brief initialize an SPI message structure
 * @param msg: the message to initialize
 */
void spi_msg_init(struct spi_msg *msg);


#endif /* SPCTLR_H */
