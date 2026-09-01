/**
 * @file include/ad7814.h
 *
 * @brief interface to the demo AD7814 SPI temperature sensor driver
 */

#ifndef AD7814_H
#define AD7814_H


/**
 * @brief read the current temperature from the AD7814 sensor
 * @return temperature in degrees Celsius
 */
float ad7814_get_temp(void);

/**
 * @brief register the AD7814 driver with a chip select callback
 * @param chip_select: callback to assert/deassert chip select (or NULL)
 * @return 0 on success, negative error code on failure
 */
int ad7814_register(void (*chip_select)(bool));

#endif /* AD7814_H */
