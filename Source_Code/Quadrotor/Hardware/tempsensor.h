#ifndef _TEMPSENSOR_H
#define _TEMPSENSOR_H

#include "stm32f10x.h"

#define  ADC_Channel_tempsensor  ADC_Channel_16 //�¶ȴ�����ͨ��

void T_adc_init(void);//ADCͨ����ʼ��
u16  T_get_adc(u8 channel);//��ȡĳ��ͨ��ֵ
u16  T_get_adc_average(u8 channel,u8 times);//�õ�ĳ��ͨ��10�β�����ƽ��ֵ

#endif
