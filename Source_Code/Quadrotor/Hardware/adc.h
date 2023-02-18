#ifndef _ADC_H
#define _ADC_H

#include "sys.h"

void adc1_init(void);
void adc_dma_start(void);
u16  get_once_ADC(void);
u16  get_adc_average(u8 channel,u8 repeat);

#endif
