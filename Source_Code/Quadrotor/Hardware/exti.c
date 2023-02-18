#include "exti.h"
#include "key.h"
#include "delay.h"

//key0---PC5    key1---PA15(与JTAG共用)    wk_up---PA0


extern u8 key1_value;


//飞机上:  key1---PA1
void exti_init(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);//外部中断, 需要使能AFIO时钟	
	
	key_init();//初始化按键对应io模式


	/* 选择某GPIO管脚作为外部中断 */
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);	//key1
//	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource11);	//key2
//	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource9 );	//key3
	
	/* 初始化外部中断寄存器 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line1 /*| EXTI_Line11 | EXTI_Line9*/;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;	//下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);



	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;		//使能key1(PA1)
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//使能
	NVIC_Init(&NVIC_InitStructure); 

//	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;			//使能key3(PB9)所在的外部中断通道
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//抢占优先级2 
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//子优先级1
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//使能
//	NVIC_Init(&NVIC_InitStructure);
}



//key1---PA1
void EXTI1_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line1)!=RESET)
	{
		delay_ms(7);//消抖
		if(key1==0)	//再次判断(低电平)---现在非常完美!
			key1_value += 1;
		else ;
		if(key1_value == 2)
			key1_value = 0;
		else ;
	}
	EXTI_ClearITPendingBit(EXTI_Line1);
}
/*
void EXTI9_5_IRQHandler(void)
{
	delay_ms(10);//消抖
	if(key0==0)//if(EXTI_GetITStatus(EXTI_Line5)!=RESET)
	{	  
		led0=!led0;
	}
	EXTI_ClearITPendingBit(EXTI_Line5);//清除LINE5上的中断标志位
}

void EXTI15_10_IRQHandler(void)
{
	delay_ms(10);//消抖
	if(key1==0)
	{	  
		led1=!led1;	
	}
	EXTI_ClearITPendingBit(EXTI_Line15);//清除LINE15上的中断标志位
}
*/


