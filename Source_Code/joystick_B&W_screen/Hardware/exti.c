#include "exti.h"
#include "key.h"
#include "delay.h"

//key0---PC5    key1---PA15(��JTAG����)    wk_up---PA0


//ң����V1.0:  key1---PA15(����JATG_TestDataInput(JTDI))     key2---PB11     key3_user---PB9
//ң����V2.0:  key1---PB8(δ����JATG, ������JTAG����ʵ��)	key2---PB11		key3_user1---PC3	key4_user2---PB10
void exti_init(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);//�ⲿ�ж�, ��Ҫʹ��AFIOʱ��	
	
	key_init();//��ʼ��������Ӧioģʽ


	/* ѡ��ĳGPIO�ܽ���Ϊ�ⲿ�ж� */
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8 );	//key1
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource11);	//key2
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource3 );	//key3
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource10);	//key4
	
	/* ��ʼ���ⲿ�жϼĴ��� */
	EXTI_InitStructure.EXTI_Line = EXTI_Line8 | EXTI_Line11 | EXTI_Line3 | EXTI_Line10;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;	//�½��ش���
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);



	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;		//ʹ�� key2(PB11), key4_user2(PB10)���ⲿ�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//�����ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//ʹ��
	NVIC_Init(&NVIC_InitStructure); 

	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;			//ʹ�� key1(PB8)���ڵ��ⲿ�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//��ռ���ȼ�2 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//�����ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//ʹ��
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;			//ʹ�� key3_user1(PC3)���ڵ��ⲿ�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//��ռ���ȼ�2 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//�����ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//ʹ��
	NVIC_Init(&NVIC_InitStructure);
}


/*
//key0---PC5    key1---PA15(��JTAG����)    wk_up---PA0
void EXTI0_IRQHandler(void)
{
	delay_ms(10);//����
	//������жϻ�����д��  if(wk_up==1)
	if(EXTI_GetITStatus(EXTI_Line0)!=RESET)//�ж�LINE0�ϵ��ж��Ƿ���
	{	  
		led0=!led0;
		led1=!led1;	
	}
	EXTI_ClearITPendingBit(EXTI_Line0);//���LINE0�ϵ��жϱ�־λ(���EXTI0��·����λ)
}

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
