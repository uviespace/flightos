#ifndef AD7814_H
#define AD7814_H


float ad7814_get_temp(void);
int ad7814_register(void (*chip_select)(bool));

#endif /* AD7814_H */
