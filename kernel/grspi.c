/**
 * @file kernel/grspi.c
 *
 * @brief a driver for the Gaisler SPI controller
 *
 * @note this was only tested with the GR712RC
 *
  */


#include <kernel/module.h>
#include <kernel/kernel.h>
#include <kernel/printk.h>

#include <kernel/kmem.h>
#include <kernel/err.h>
#include <kernel/err.h>
#include <kernel/irq.h>
#include <asm-generic/io.h>
#include <asm-generic/irqflags.h>
#include <asm-generic/spinlock.h>


#include <grspi.h>
#include <kernel/spictlr.h>

#define MSG "GRSPI: "



static void grspi_rx_buf_8(uint32_t data, struct grspi *grspi)
{
	(*(uint8_t *)grspi->rx++) = (uint8_t)(data >> grspi->chip.rx_shift);
}

static void grspi_rx_buf_16(uint32_t data, struct grspi *grspi)
{
	(*(uint16_t *)grspi->rx++) = (uint16_t)(data >> grspi->chip.rx_shift);
}

static void grspi_rx_buf_32(uint32_t data, struct grspi *grspi)
{
	(*(uint32_t *)grspi->rx++) = (uint32_t)(data >> grspi->chip.rx_shift);
}

static uint32_t grspi_tx_buf_8(struct grspi *grspi)
{
	return (*(uint8_t *)grspi->tx++) << grspi->chip.tx_shift;
}

static uint32_t grspi_tx_buf_16(struct grspi *grspi)
{
	return (*(uint16_t *)grspi->tx++) << grspi->chip.tx_shift;
}

static uint32_t grspi_tx_buf_32(struct grspi *grspi)
{
	return (*(uint32_t *)grspi->tx++) << grspi->chip.tx_shift;
}





/* XXX TODO need to implement a driver model + probing at some
 * point; for now, I'll just hard-code this here and use a single global
 * storage for the device info
 */

static struct grspi *grpspi_get_device(void)
{
	static struct grspi *dev;


	if (!dev) {
		dev = kcalloc(1, sizeof(*dev));
		if (!dev)
			goto exit;
	}

	/* values for GR712RC, see GR712RC-UM, Mar 2025, Version 2.17 */
	dev->regs = (void *)0x80000400;
	dev->irq  = 13;
	dev->word_nbits_max = 32;
	dev->n_chipsel = 0;	/* no chipselects in the GR712RC */

exit:
	return dev;
}


static void grspi_write_register(uint32_t *reg, uint32_t data)
{
	iowrite32be(data, reg);
}

static uint32_t grspi_read_register(uint32_t *reg)
{
	return ioread32be(reg);
}




static void grspi_set_clk(struct grspi_chip *chip, uint32_t speed_hz)
{
        uint32_t div;

/* shut up toolchain warnings */
#ifdef CONFIG_CPU_CLOCK_FREQ
#define AMBA_CLOCK	CONFIG_CPU_CLOCK_FREQ
#else
#define AMBA_CLOCK 0
#endif

	/* SPICLK is the system clock divided by 4 * (PM + 1) if FACT==0,
	 * so DIV = PM + 1 -> PM = DIV - 1
	 * therefore
	 * DIV = (AMBA_CLOCK / 4 + speed_hz) / speed_hz
	 */

	/* FACT==1 is fine, as we do not need MPC83xx compatibility, so
	 * we start searching for the divisor which gives us the highest
	 * possible SPICLK without going over speed_hz; since this is an
	 * integer division, we force a round down by subtracting 1
	 * from the dividend
	 */
        div = ((AMBA_CLOCK >> 1) + speed_hz - 1) / speed_hz;

        if (div > 16) {
                div = (div + 15) / 16;	/* also round down remainder */
		chip->mode_reg |= GRSPI_MODE_DIV16;
        }

        if (div < 16)
		chip->mode_reg |= GRSPI_MODE_FACT;	/* we're good, use computed divider w/ FACT */
	else
                div = (div + 1) >> 1;	/* need another div/2; this time round up for the next highest frequency */


	chip->mode_reg |= GRSPI_MODE_PM(div - 1);
}



static void grspi_set_shifts(struct grspi_chip *chip, int bits_per_word, int rev)
{
	/* 32 bit default */
	chip->rx_shift = 0;
	chip->tx_shift = 0;

	if (!rev) {	/* no reverse data: lsb first */
		if (bits_per_word <= 8) {
			chip->rx_shift = 8;
			chip->tx_shift = 0;
		}
	} else {

		if (bits_per_word <= 8) {
			chip->rx_shift = 16;
			chip->tx_shift = 24;
		} else if (bits_per_word <= 16) {
			chip->rx_shift = 16;
			chip->tx_shift = 16;
		}
	}
}



static void grspi_setup_transfer(struct spi_dev *spi, struct spi_transfer *t)
{
	uint32_t hz = 0;
	int bits_per_word = 0;

	struct grspi *grspi;
	struct grspi_chip *chip;


	chip = spi->ctlr_data;
	grspi = spi->ctlr->ctlr_dev;

	if (t) {
		hz = t->speed_hz;
		bits_per_word = t->bits_per_word;
	}

	if (!hz)
		hz = spi->max_speed_hz;
	if (!bits_per_word)
		bits_per_word = spi->bits_per_word;


	if (bits_per_word <= 8) {
		chip->get_rx = grspi_rx_buf_8;
		chip->get_tx = grspi_tx_buf_8;
	} else if (bits_per_word <= 16) {
		chip->get_rx = grspi_rx_buf_16;
		chip->get_tx = grspi_tx_buf_16;
	} else if (bits_per_word <= 32) {
		chip->get_rx = grspi_rx_buf_32;
		chip->get_tx = grspi_tx_buf_32;
	}

	grspi_set_shifts(chip, bits_per_word, spi->mode & SPI_MSB_FIRST);

	/* store settings of active chip */
	grspi->chip = (*chip);

	/* LEN field in GRSPI mode register: 32 => 0, else (word length - 1) */
	if (bits_per_word == 32)
		bits_per_word = 0;
	else
		bits_per_word -= 1;

	/* mask out configurable fields */
	chip->mode_reg &= ~(GRSPI_MODE_DIV16 | GRSPI_MODE_LEN(0xF) | GRSPI_MODE_PM(0xF) | GRSPI_MODE_FACT);

	chip->mode_reg |= GRSPI_MODE_LEN(bits_per_word);

	grspi_set_clk(chip, hz);


		/* Turn off IRQs locally to minimize time that SPI is disabled. */

	arch_local_irq_disable();

	/* GSRPI must be disabled when changing modes */
	grspi_write_register(&grspi->regs->mode, chip->mode_reg & ~GRSPI_MODE_EN);
	grspi_write_register(&grspi->regs->mode, chip->mode_reg |  GRSPI_MODE_EN);
	arch_local_irq_enable();
}


static int grspi_bufs_sync(struct spi_dev *spi, struct spi_transfer *t)
{
	uint32_t evt;
	uint32_t word;
	uint32_t data;
	struct grspi *grspi;

	unsigned int len = t->len;
	uint8_t bits_per_word;

	grspi = spi->ctlr->ctlr_dev;


	bits_per_word = spi->bits_per_word;
	if (t->bits_per_word)
		bits_per_word = t->bits_per_word;

	if (bits_per_word > 8)
		len /= 2;
	if (bits_per_word > 16)
		len /= 2;

	grspi->tx = t->tx_buf;
	grspi->rx = t->rx_buf;

	grspi->count = len;

	spi->chip_select(1);
	/* disable RX IRQ */
	grspi_write_register(&grspi->regs->msk, 0);

	/* transmit word */
	word = grspi->chip.get_tx(grspi);
	grspi_write_register(&grspi->regs->tx, word);

	do {	/* wait for RX queue to not be empty */
		evt = grspi_read_register(&grspi->regs->evt);
	} while (!(evt & GRSPI_EVT_NF));

	/* data received, update */
	data = grspi_read_register(&grspi->regs->rx);
	if (grspi->rx)
		grspi->chip.get_rx(data, grspi);

	/* clear event register */
	grspi_write_register(&grspi->regs->evt, evt);

	spi->chip_select(0);

	return grspi->count;
}



static int done;
__attribute__((unused))
static int grspi_bufs_async(struct spi_dev *spi, struct spi_transfer *t)
{
	struct grspi *grspi;
	uint32_t word;

	unsigned int len = t->len;
	uint8_t bits_per_word;

	grspi = spi->ctlr->ctlr_dev;


	bits_per_word = spi->bits_per_word;
	if (t->bits_per_word)
		bits_per_word = t->bits_per_word;

	if (bits_per_word > 8)
		len /= 2;
	if (bits_per_word > 16)
		len /= 2;

	grspi->tx = t->tx_buf;
	grspi->rx = t->rx_buf;

	//grspi->done = 0;
	done = 0;

	grspi->count = len;

	spi->chip_select(1);
	/* enable RX IRQ */
	grspi_write_register(&grspi->regs->msk, GRSPI_MSK_NEE);

	/* transmit word */
	word = grspi->chip.get_tx(grspi);
	grspi_write_register(&grspi->regs->tx, word);

	while(!ioread32be(&done));
//	while (!grspi->done);

	/* disable RX IRQ */
	grspi_write_register(&grspi->regs->msk, 0);
	spi->chip_select(0);

	return grspi->count;
}






static int grspi_transfer_one(struct spi_ctlr *ctlr,
			      struct spi_dev *spi,
			      struct spi_transfer *t)
{
	int status = 0;

	grspi_setup_transfer(spi, t);


	if (t->len)
		status = grspi_bufs_sync(spi, t);
	if (status > 0)
		return -EMSGSIZE;

	return status;
}


static int grspi_setup(struct spi_dev *spi)
{
	struct grspi *grspi;
	struct grspi_chip *chip;


	chip = spi->ctlr_data;

	if (!spi->max_speed_hz)
		return -EINVAL;

	if (!chip) {	/* register run time state on first pass */
		chip = kzalloc(sizeof(*chip));
		if (!chip)
			return -ENOMEM;

		spi->ctlr_data = chip;
	}



	grspi = spi->ctlr->ctlr_dev;

	chip->mode_reg = grspi_read_register(&grspi->regs->mode);

	chip->mode_reg &= ~(GRSPI_MODE_LOOP | GRSPI_MODE_CPOL | GRSPI_MODE_CPHA | GRSPI_MODE_REV);

	if (spi->mode & SPI_LOOP)
		chip->mode_reg |= GRSPI_MODE_LOOP;

	if (spi->mode & SPI_CPOL)
		chip->mode_reg |= GRSPI_MODE_CPOL;

	if (spi->mode & SPI_CPHA)
		chip->mode_reg |= GRSPI_MODE_CPOL;

	if (spi->mode & SPI_MSB_FIRST)
		chip->mode_reg |= GRSPI_MODE_REV;

	grspi_setup_transfer(spi, NULL);

	return 0;
}


__attribute__((unused))
static irqreturn_t grspi_irq(unsigned int irq, void *userdata)
{
	uint32_t evt;
	uint32_t data;

	struct grspi *spi = userdata;



	evt = grspi_read_register(&spi->regs->evt);
	if (!evt)
		return IRQ_NONE;

	/* TODO: add queues as in tty implementation
	 *
	 * in SPI we have to write as many bits as we expect to
	 * fall out on the data output, so if our device requires
	 * a 16-bit frame but only an 8 bit command,  we clock in
	 * <8 bit value><8 bit 0>  and receive <16 bit value>
	 */


	if (evt & GRSPI_EVT_NE) { /* rx data in queue? */


		data = grspi_read_register(&spi->regs->rx);
		if (spi->rx)
			spi->chip.get_rx(data, spi);
	}

	if (!(evt & GRSPI_EVT_NF)) {
		do {	/* wait for tx queue to be able to accept one more word */
			evt = grspi_read_register(&spi->regs->evt);
		} while (!(evt & GRSPI_EVT_NF));
	}

	/* clear event register */
	grspi_write_register(&spi->regs->evt, evt);
#if 0
	spi->count--;
	if (spi->count)
		grspi_write_register(&spi->regs->tx, spi->chip.get_tx(spi));
#endif
	done = 1;
	return IRQ_HANDLED;

}

static int grspi_init_dev(void)
{
	int ret;

	uint32_t mode;

	struct grspi *spi;
	struct spi_ctlr *ctlr;



	ctlr = kzalloc(sizeof(*ctlr));
	if (!ctlr) {
		ret = -ENOMEM;
		goto exit;
	}


	spi = grpspi_get_device();
	if (!spi) {
		ret = -ENOMEM;
		goto exit;
	}


	mode = grspi_read_register(&spi->regs->mode);
	mode |= GRSPI_INIT_MODE;	/* set default configuration */
	grspi_write_register(&spi->regs->mode, mode);

	ctlr->ctlr_dev = spi;
	ctlr->setup = grspi_setup;
#if 0
	ctlr->cleanup = fsl_spi_cleanup;
	ctlr->prepare_message = fsl_spi_prepare_message;
	ctlr->unprepare_message = fsl_spi_unprepare_message;
	ctlr->use_gpio_descriptors = true;
	ctlr->set_cs = fsl_spi_cs_control;
	ctlr->transfer_one_msg = grspi_transfer_one_msg;
#endif
	ctlr->transfer_one = grspi_transfer_one;
	ctlr->bus_num = 0;

	spi_register_ctlr(ctlr);

	/* XXX DISABLED FOR NOW; this works, but we won't need it at this time */
#if 0
	ret = irq_request(spi->irq, ISR_PRIORITY_NOW, grspi_irq, spi);
	if (ret)
		goto exit;

#endif

exit:
	return ret;
}


#if 0
/**
 * @brief driver cleanup function
 */

static void grspi_exit(void)
{
	printk(MSG "module_exit()\n");
}
module_exit(grspi_exit);
#endif

/**
 * @brief driver initialisation
 */

static int grspi_init(void)
{
	pr_info(MSG "module_init()\n");

	grspi_init_dev();

	return 0;
}

module_init(grspi_init);
