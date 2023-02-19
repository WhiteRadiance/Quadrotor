#include "wkup.h"
#include "led.h"
#include "delay.h"

void sys_standby(void)
{  
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);	//ʹ��PWR����ʱ��
	PWR_WakeUpPinCmd(ENABLE);//ʹ�ܻ��ѹܽŹ���
	PWR_EnterSTANDBYMode();//�������(STANDBY)ģʽ 		 
}

//ϵͳ�������ģʽ
void sys_enter_standby(void)
{			 
	RCC_APB2PeriphResetCmd(0X01FC,DISABLE);//��λ����IO��_ABCDEFG
	sys_standby();
}

//���WKUP�ŵ��ź�
//����ֵ1:��������3s����
//      0:����Ĵ���
u8 check_wkup(void)//������wk_up,�����ͣ��while(1)��...............................
{
	u8 t = 0;//��¼���µ�ʱ��
	led0 = 0;//����DS0
	while(1)
	{
		if(wk_up)
		{
			t++;//�Ѿ�������
			delay_ms(30);
			if(t >= 100)//���³���3����
			{
				led0 = 0;//����DS0 
				return 1;//����3s������
			}
		}
		else
		{ 
			led0 = 1;
			return 0;//���²���3��
		}
	}
}

//�ж�,��⵽PA0�ŵ�һ��������.	  
//�ж���0���ϵ��жϼ��
void EXTI0_IRQHandler(void)
{ 		    		    				     		    
	EXTI_ClearITPendingBit(EXTI_Line0);//���LINE10�ϵ��жϱ�־λ		  
	if(check_wkup())//����(�ػ�)?
	{		  
		sys_enter_standby();
	}
}

//PA0 WKUP���ѳ�ʼ��
void wkup_init(void)
{	
  GPIO_InitTypeDef  GPIO_InitStructure;  		  
	NVIC_InitTypeDef 	NVIC_InitStructure;
	EXTI_InitTypeDef 	EXTI_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);//ʹ��GPIOA�͸��ù���ʱ��

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;//PA.0:wkup��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;//��������
	GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��IO
  //ʹ���ⲿ�жϷ�ʽ
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);	//�ж���0����GPIOA.0

  EXTI_InitStructure.EXTI_Line = EXTI_Line0;//���ð������е��ⲿ��·
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//�����ⲿ�ж�ģʽ:EXTI��·Ϊ�ж�����
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//�����ش���
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);//��ʼ���ⲿ�ж�

	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn; //ʹ�ܰ������ڵ��ⲿ�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; //��ռ���ȼ�2��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2; //�����ȼ�2��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //ʹ���ⲿ�ж�ͨ��
	NVIC_Init(&NVIC_InitStructure); //����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���

	if(check_wkup()==0)//���غ����ͣ��check_wkup��,�ȴ�����0��1,�����1�������ⲽֱ����ʾLCD��
		sys_standby();//���ǿ���,�������ģʽ
}

