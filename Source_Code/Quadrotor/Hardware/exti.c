#include "exti.h"
#include "key.h"
#include "delay.h"

//key0---PC5    key1---PA15(��JTAG����)    wk_up---PA0


extern u8 key1_value;


//�ɻ���:  key1---PA1
void exti_init(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);//�ⲿ�ж�, ��Ҫʹ��AFIOʱ��	
	
	key_init();//��ʼ��������Ӧioģʽ


	/* ѡ��ĳGPIO�ܽ���Ϊ�ⲿ�ж� */
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);	//key1
//	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource11);	//key2
//	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource9 );	//key3
	
	/* ��ʼ���ⲿ�жϼĴ��� */
	EXTI_InitStructure.EXTI_Line = EXTI_Line1 /*| EXTI_Line11 | EXTI_Line9*/;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;	//�½��ش���
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);



	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;		//ʹ��key1(PA1)
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//ʹ��
	NVIC_Init(&NVIC_InitStructure); 

//	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;			//ʹ��key3(PB9)���ڵ��ⲿ�ж�ͨ��
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//��ռ���ȼ�2 
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//�����ȼ�1
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//ʹ��
//	NVIC_Init(&NVIC_InitStructure);
}



//key1---PA1
void EXTI1_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line1)!=RESET)
	{
		delay_ms(7);//����
		if(key1==0)	//�ٴ��ж�(�͵�ƽ)---���ڷǳ�����!
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
	delay_ms(10);//����
	if(key0==0)//if(EXTI_GetITStatus(EXTI_Line5)!=RESET)
	{	  
		led0=!led0;
	}
	EXTI_ClearITPendingBit(EXTI_Line5);//���LINE5�ϵ��жϱ�־λ
}

void EXTI15_10_IRQHandler(void)
{
	delay_ms(10);//����
	if(key1==0)
	{	  
		led1=!led1;	
	}
	EXTI_ClearITPendingBit(EXTI_Line15);//���LINE15�ϵ��жϱ�־λ
}
*/


