#include "led.h"
#include "stm32f10x.h"

//led1---PB0		led2---PB1			led3---PB10

//GPIOB / RCC_APB2Periph_GPIOB / GPIO_Pin_X 在led.h中进行宏定义
void led_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(LED1_GPIO_CLK | LED2_GPIO_CLK | LED3_GPIO_CLK, ENABLE);//enable GPIOX clock
	
	GPIO_InitStructure.GPIO_Pin = LED1_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(LED1_GPIO_PORT, &GPIO_InitStructure);		//init LED1(PB0)
	
	GPIO_InitStructure.GPIO_Pin = LED2_GPIO_PIN;
	GPIO_Init(LED2_GPIO_PORT, &GPIO_InitStructure);		//init LED2(PB1)
	
	GPIO_InitStructure.GPIO_Pin = LED3_GPIO_PIN;
	GPIO_Init(LED3_GPIO_PORT, &GPIO_InitStructure);		//init LED3(PB10)
	
	GPIO_SetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);		//Set HIGH to be invalid
	GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);		//Set HIGH to be invalid
	GPIO_SetBits(LED3_GPIO_PORT, LED3_GPIO_PIN);		//Set HIGH to be invalid
}
