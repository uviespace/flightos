#include <errno.h>
#include <limits.h>
#include <kernel/printk.h>
#include <kernel/spictlr.h>


#if defined(DEMO_AD7814)

/**
 * example usage with GR712RC eval board
 * - insert JP71 for chip select via GPIO54
 * - make sure GPIO54 is connected to actual GPIO line:
 *	set JP54 to GPIO column
 * * - make sure SPICLK, SPIMOSI, SPIMISO are connected:
 *	set JP44, JP45, JP51 to either CF0 or CF4 column
 *
 * see GR712 Development Board User Manual, February 2012, Rev. 0.7
 * p 13, fig 2-5 switch matrix
 * p 28, fig 2-22 spi configuration
 * p 50, fig 4-2 pcb assembly for jumper locations;
 *	note: CF0 == leftmost, CF4 == rightmost pin columns
 *
 */

#include <ad7814.h>
#include <asm/leon_reg.h>
#include <asm-generic/io.h>
#include <kernel/kthread.h>
#include <kernel/init.h>
#include <kernel/user.h>

static void gr712_ad7814_spi_cs(bool enable)
{
	uint32_t dir;
	uint32_t out;

	struct leon3_grgpio_registermap *reg = (void *)LEON3_BASE_ADDRESS_GRGPIO_2;


	dir = ioread32be(&reg->ioport_direction);
	out = ioread32be(&reg->ioport_output_value);

	/* CS on AD7814 is active low */
	if (enable) {
		iowrite32be(dir |  (1 << 22), &reg->ioport_direction);
		iowrite32be(out & ~(1 << 22), &reg->ioport_output_value);
	} else {
		iowrite32be(out |  (1 << 22), &reg->ioport_output_value);
		iowrite32be(dir & ~(1 << 22), &reg->ioport_direction);
	}
}


static int ad7814_poll_thread(void *data)
{
	while (1) {
		printk("%.2f\n", ad7814_get_temp());
		sched_yield();
	}

	return 0;
}

static int ad7814_poll_init(void)
{
	struct task_struct *t;


	ad7814_register(gr712_ad7814_spi_cs);

	t = kthread_create(ad7814_poll_thread, NULL, KTHREAD_CPU_AFFINITY_NONE, "POLL_AD7814_TEMP");
	BUG_ON(!t);

	/* run for at most 2 ms every 125 ms (~0.8 % CPU) */
	kthread_set_sched_edf(t, 250 * 1000, 249*1000, 2 * 1000);

	BUG_ON(kthread_wake_up(t) < 0);

	return 0;
}
lvl1_usercall(ad7814_poll_init)

#endif /* DEMO_AD7814 */




static void (*cs)(bool);
static struct spi_dev *spi_dev;
static uint16_t rx_raw;
static const uint16_t tx_cmd = 0x0;


static void ad7814_cs(bool enable)
{
	if (!cs)
		return;

	cs(enable);
}


static struct spi_board_info ad7814_info = {
	.max_speed_hz = 10000000,
	.chip_select  = ad7814_cs,
	.mode	      = (SPI_MODE_3 | SPI_MSB_FIRST),
};

static void ad7814_poll(void)
{
	struct spi_transfer t = {
		.tx_buf = &tx_cmd,
		.rx_buf = &rx_raw,
		.len    = sizeof(tx_cmd),
		.bits_per_word = sizeof(tx_cmd) * CHAR_BIT,
	};

	if(!spi_dev)
		return;

	spi_sync_transfer(spi_dev, &t, 1);
}



/**
 * @brief returns the temperature in degrees celsius
 */

float ad7814_get_temp(void)
{
	int16_t t;


	ad7814_poll();

	/* I don't get why I only need a shift of 4 here; the value should
	 * be shifted out after 11 clocks, with a leading 0-bit (which appears
	 * as either 0 or 1 occasionally) + 10 value bits which encode 0.25°C
	 * per bit as two's complement
	 *
	 * For some reason, the shifted in values land at a 4-bit offset from
	 * the LSB position rather than at 5 bits; maybe this is caused by the
	 * grspi controller.
	 * note: the remaining "filler" bits will have the value of the actual
	 * value's LSB
	 *
	 */

	/* we left-shift once so we can sign-extend, then right for 4+1 */
	t = (int16_t)(rx_raw << 1) >> 5;

	/* negative values are actually untested, need put the board into
	 * a climate chamber to verify this working.
	 */
	if (t & 0x200)
		t -= 0x200;

	return (float)t * 0.25;
}


/**
 * @brief register an AD7814 for operation
 *
 * @note: for now, the user must supply a chip select function for this
 * to work
 */

int ad7814_register(void (*chip_select)(bool))
{
	if (spi_dev)
		return -EADDRINUSE;

	if (!chip_select)
		return -EINVAL;

	spi_dev = spi_new_dev(spi_get_master(0), &ad7814_info);
	if (!spi_dev)
		return -ENODEV;

	cs = chip_select;

	ad7814_get_temp();	/* prime device */


	return 0;
}


/**
 * @brief de-register the AD7814
 */

int ad7814_deregister(void)
{
	if (!spi_dev)
		return -ENODEV;

	spi_release_dev(spi_dev);

	spi_dev = NULL;

	return 0;
}
