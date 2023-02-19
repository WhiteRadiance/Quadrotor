#ifndef _TEMPSENSOR_H
#define _TEMPSENSOR_H

#include "stm32f10x.h"

#define  ADC_Channel_tempsensor  ADC_Channel_16 //温度传感器通道

void T_adc_init(void);//ADC通道初始化
u16  T_get_adc(u8 channel);//获取某个通道值
u16  T_get_adc_average(u8 channel,u8 times);//得到某个通道10次采样的平均值

#endif
