/**
 * @file   kernel/spictlr.h
 * @ingroup grspw2
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



#define	SPI_CPHA		(1 << 0)	/* clock phase */
#define	SPI_CPOL		(1 << 1)	/* clock polarity */
#define	SPI_MSB_FIRST		(1 << 2)	/* bit clock out order */
#define	SPI_LOOP		(1 << 3)	/* test looback */

/* the standard modes */
#define	SPI_MODE_0		(0 | 0)			/* idle polarity low, sample on rising edge, shift out on falling edge */
#define	SPI_MODE_1		(0 | SPI_CPHA)		/* idle polarity low, sample on falling edge, shift out on rising edge */
#define	SPI_MODE_2		(SPI_CPOL | 0)		/* idle polarity high, sample on falling edge, shift out on rising edge */
#define	SPI_MODE_3		(SPI_CPOL | SPI_CPHA)	/* idle polarity high, sample on rising edge, shift out on falling edge */



struct spi_msg {
	struct spi_dev		*spi;
	struct list_head        transfers;
	int			status;
	void			*userdata;
	void (*complete)(void *userdata);
};

struct spi_transfer {
	const void	*tx_buf;
	void		*rx_buf;
	uint32_t	len;
	uint32_t	speed_hz;
	uint8_t		bits_per_word;

	struct list_head transfer_list;
};


struct spi_ctlr {

	void	*ctlr_dev;
	uint8_t	 bus_num;

	int (*setup)(struct spi_dev *spi);
	int (*transfer_one)(struct spi_ctlr *ctlr,
			    struct spi_dev *spi,
			    struct spi_transfer *transfer);

	struct spinlock bus_lock_spinlock;
	struct list_head node;
};


struct spi_dev {

	struct spi_ctlr	*ctlr;
	void		*ctlr_data;
	uint32_t	max_speed_hz;
	uint32_t	mode;

	void (*chip_select)(bool enable);

	uint8_t		irq;
	uint8_t		bits_per_word;
};


struct spi_board_info {

	uint32_t mode;
	uint32_t max_speed_hz;

	/* we currently target only the GR712RC SPI controller, so we
	 * provide a chip select call, rather than a number for this
	 * board; if a chip select is available in an other controller
	 * add a "int num_chip_select" here and check whether chip_select
	 * is NULL to determine which mechanism to use
	 */
	void (*chip_select)(bool enable);
	uint8_t irq;
};


int spi_register_ctlr(struct spi_ctlr *ctlr);
struct spi_ctlr *spi_get_master(uint8_t bus);

struct spi_dev *spi_new_dev(struct spi_ctlr *ctlr, struct spi_board_info *chip);
void spi_release_dev(struct spi_dev *dev);

int spi_setup(struct spi_dev *spi);

/* use for multiple blocking transfer(s); the SPI target is unlocked and the chip
 * select is disabled between inidividual transfers
 */
int spi_sync_transfer(struct spi_dev *spi, struct spi_transfer *xfers, unsigned int num_xfers);

/* a single non-blocking tranfer, can be used from in ISR; when transfer is finalised,
 * the result will be propagated via the complete() call in messsage configuration
 */
int spi_async(struct spi_dev *spi, struct spi_msg *msg);

/* single blocking transfer */
int spi_sync(struct spi_dev *spi, struct spi_msg *msg);

void spi_msg_init(struct spi_msg *msg);


#endif /* SPCTLR_H */
