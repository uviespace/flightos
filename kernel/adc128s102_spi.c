#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <kernel/printk.h>
#include <kernel/spictlr.h>
#include <kernel/sysctl.h>
#include <adc128s102.h>

#if defined(DEMO_ADC128S102)

#include <asm/leon_reg.h>
#include <asm-generic/io.h>
#include <kernel/kthread.h>
#include <kernel/init.h>
#include <kernel/user.h>



static void gr712_adc128s102_spi_cs(bool enable)
{
	uint32_t dir;
	uint32_t out;

	struct leon3_grgpio_registermap *reg = (void *)LEON3_BASE_ADDRESS_GRGPIO_2;


	dir = ioread32be(&reg->ioport_direction);
	out = ioread32be(&reg->ioport_output_value);

	/* CS on ADC128S102 is active low */
	if (enable) {
		iowrite32be(dir |  (1 << 22), &reg->ioport_direction);
		iowrite32be(out & ~(1 << 22), &reg->ioport_output_value);
	} else {
		iowrite32be(out |  (1 << 22), &reg->ioport_output_value);
		iowrite32be(dir & ~(1 << 22), &reg->ioport_direction);
	}
}

static int adc128s102_poll_thread(void *data)
{
	int i;

	while (1) {

		for (i = 0; i < 8; i++)
			printk("ADC[%d] 0x%04x\n", i, adc128s102_get_value(i));

		sched_yield();
	}

	return 0;
}

static int adc128s102_poll_init(void)
{
	struct task_struct *t;


	adc128s102_register(gr712_adc128s102_spi_cs);

	t = kthread_create(adc128s102_poll_thread, NULL, KTHREAD_CPU_AFFINITY_NONE, "POLL_ADC128S102_TEMP");
	BUG_ON(!t);

	/* run for at most 2 ms every 125 ms (~0.8 % CPU) */
	kthread_set_sched_edf(t, 250 * 1000, 249*1000, 2 * 1000);

	BUG_ON(kthread_wake_up(t) < 0);

	return 0;
}
lvl1_usercall(adc128s102_poll_init)

#endif /* DEMO_ADC128S102 */


/* we have to clock in 16 bits, channel address is 3 bits:
 * 0-1 : don't care
 * 2-4 : channel address
 * 4-15: don't care
 *
 * 12bit ADC output is:
 * 0-3 :  0
 * 4-15: sample value
 */


static void (*cs)(bool);
static struct spi_dev *spi_dev;
static uint16_t rx_raw[8];
static const uint16_t tx_cmd[8] = {
	0 << (3 + 8),
	1 << (3 + 8),
	2 << (3 + 8),
	3 << (3 + 8),
	4 << (3 + 8),
	5 << (3 + 8),
	6 << (3 + 8),
	7 << (3 + 8),
};


#if defined(CONFIG_SYSCTL)
#define MAX_CHARS_PER_NAME 2
static char adc128s102_names[8 * MAX_CHARS_PER_NAME];
static struct sobj_attribute  adc128s102_attr[8];
static struct sobj_attribute *adc128s102_attributes[8 + 1];

static ssize_t adc128s102_show(__attribute__((unused)) struct sysobj *sobj,
			__attribute__((unused)) struct sobj_attribute *sattr,
			char *buf)
{
	int channel;


	channel = strtol(sattr->name, NULL, 10) & 0x7;

	return sprintf(buf, "%u", adc128s102_get_value(channel));
}

static int adc128s10_init_sysctl(void)
{
	size_t i;
	struct sysobj *sobj;

	sobj = sysobj_create();

	if (!sobj)
		return -ENOMEM;

	for (i = 0; i < 8; i++) {

		snprintf(&adc128s102_names[i * MAX_CHARS_PER_NAME], MAX_CHARS_PER_NAME,"%u", i);
		adc128s102_attr[i].name  = &adc128s102_names[i * MAX_CHARS_PER_NAME];
		adc128s102_attr[i].show  = adc128s102_show;
		adc128s102_attr[i].store = NULL;

		adc128s102_attributes[i] = &adc128s102_attr[i];
	}

	/* be explicit; last item in attribute pointer list is always NULL */
	adc128s102_attributes[8] = NULL;


	sobj->sattr = adc128s102_attributes;
	sysobj_add(sobj, NULL, sysctl_root(), "adc128s102");

	return 0;
}
#endif /* CONFIG_SYSCTL */


static void adc128s102_cs(bool enable)
{
	if (!cs)
		return;

	cs(enable);
}


static struct spi_board_info adc128s102_info = {
	.max_speed_hz = 16000000,
	.chip_select  = adc128s102_cs,
	.mode	      = (SPI_MODE_2 | SPI_MSB_FIRST),
};

static void adc128s102_poll(uint8_t channel)
{
	struct spi_transfer t = {
		.tx_buf = &tx_cmd[channel],
		.rx_buf = &rx_raw[channel],
		.len    = sizeof(tx_cmd[0]),
		.bits_per_word = sizeof(tx_cmd[0]) * CHAR_BIT,
	};

	if(!spi_dev)
		return;

	spi_sync_transfer(spi_dev, &t, 1);
}


/**
 * @brief returns the last recorded value of an ADC channel
 */

uint16_t adc128s102_get_value(uint8_t channel)
{
	channel &= 0x7;

	adc128s102_poll(channel);

	return rx_raw[channel];
}


/**
 * @brief register an ADC128S102 for operation
 *
 * @note: for now, the user must supply a chip select function for this
 * to work
 */

int adc128s102_register(void (*chip_select)(bool))
{
	size_t i;

	if (spi_dev)
		return -EADDRINUSE;

	spi_dev = spi_new_dev(spi_get_master(0), &adc128s102_info);
	if (!spi_dev)
		return -ENODEV;

	cs = chip_select;

#if defined(CONFIG_SYSCTL)
	adc128s10_init_sysctl();
#endif /* CONFIG_SYSCTL */

	/* prime local values; the ADC128 does not need dummy polls, but
	 * we do since we will (eventually) only run in async mode
	 */
	for (i = 0; i < 8; i++)
		adc128s102_get_value(i);

	return 0;
}


/**
 * @brief de-register the ADC128S102
 */

int adc128s102_deregister(void)
{
	if (!spi_dev)
		return -ENODEV;

	spi_release_dev(spi_dev);

	spi_dev = NULL;

	return 0;
}
