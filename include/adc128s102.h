#ifndef ADC128S102_H
#define ADC128S102_H

uint16_t adc128s102_get_value(uint8_t channel);
int adc128s102_register(void (*chip_select)(bool));

#endif /* ADC128S102_H */
