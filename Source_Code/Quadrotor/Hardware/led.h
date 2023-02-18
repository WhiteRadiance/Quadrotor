#ifndef _LED_H
#define _LED_H

#include "sys.h"

#define led1 PBout(0)	//PB0���
#define led2 PBout(1)	//PB1���
#define led3 PBout(10)	//PB10���

// LED1
#define LED1_GPIO_PORT	GPIOB
#define LED1_GPIO_CLK	RCC_APB2Periph_GPIOB	//LED1��Ӧ��ʱ��
#define LED1_GPIO_PIN	GPIO_Pin_0				//PB0
// LED2
#define LED2_GPIO_PORT	GPIOB
#define LED2_GPIO_CLK	RCC_APB2Periph_GPIOB	//LED2��Ӧ��ʱ��
#define LED2_GPIO_PIN	GPIO_Pin_1				//PB1
// LED3
#define LED3_GPIO_PORT	GPIOB
#define LED3_GPIO_CLK	RCC_APB2Periph_GPIOB	//LED3��Ӧ��ʱ��
#define LED3_GPIO_PIN	GPIO_Pin_10				//PB10


void led_init(void);


#endif
