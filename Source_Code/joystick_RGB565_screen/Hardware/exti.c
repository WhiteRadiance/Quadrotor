#include "exti.h"
#include "key.h"
#include "delay.h"

//key0---PC5    key1---PA15(与JTAG共用)    wk_up---PA0


//遥控器V1.0:  key1---PA15(复用JATG_TestDataInput(JTDI))     key2---PB11     key3_user---PB9
//遥控器V2.0:  key1---PB8(未复用JATG, 但禁用JTAG方便实用)	key2---PB11		key3_user1---PC3	key4_user2---PB10
void exti_init(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);//外部中断, 需要使能AFIO时钟	
	
	key_init();//初始化按键对应io模式


	/* 选择某GPIO管脚作为外部中断 */
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8 );	//key1
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource11);	//key2
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource3 );	//key3
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource10);	//key4
	
	/* 初始化外部中断寄存器 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line8 | EXTI_Line11 | EXTI_Line3 | EXTI_Line10;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;	//下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);



	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;		//使能 key2(PB11), key4_user2(PB10)的外部中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//子优先级1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//使能
	NVIC_Init(&NVIC_InitStructure); 

	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;			//使能 key1(PB8)所在的外部中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//抢占优先级2 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//子优先级1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//使能
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;			//使能 key3_user1(PC3)所在的外部中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//抢占优先级2 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//子优先级1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//使能
	NVIC_Init(&NVIC_InitStructure);
}


/*
//key0---PC5    key1---PA15(与JTAG共用)    wk_up---PA0
void EXTI0_IRQHandler(void)
{
	delay_ms(10);//消抖
	//下面的判断还可以写成  if(wk_up==1)
	if(EXTI_GetITStatus(EXTI_Line0)!=RESET)//判断LINE0上的中断是否发生
	{	  
		led0=!led0;
		led1=!led1;	
	}
	EXTI_ClearITPendingBit(EXTI_Line0);//清除LINE0上的中断标志位(清除EXTI0线路挂起位)
}

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
