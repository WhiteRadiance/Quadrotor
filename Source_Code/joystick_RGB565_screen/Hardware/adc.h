#ifndef _ADC_H
#define _ADC_H

#include "sys.h"

void adc1_init(void);
void adc_dma_start(void);
u16  get_adc(u8 channel);

u16  adc_average(u16* val0, u16* val1, u16* val2, u16* val3);//滤波


u8 Lithium_Battery_percent(float volt);//航模锂电池的电量(%)

#endif
