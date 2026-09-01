/**
 * @file include/adc128s102.h
 *
 * @brief interface to the demo ADC128S102 SPI analog-to-digital converter driver
 */

#ifndef ADC128S102_H
#define ADC128S102_H

/**
 * @brief read an analog channel value from the ADC128S102
 * @param channel: channel number to read (0-7)
 * @return the digital conversion result
 */
uint16_t adc128s102_get_value(uint8_t channel);

/**
 * @brief register the ADC128S102 driver with a chip select callback
 * @param chip_select: callback to assert/deassert chip select (or NULL)
 * @return 0 on success, negative error code on failure
 */
int adc128s102_register(void (*chip_select)(bool));

#endif /* ADC128S102_H */
