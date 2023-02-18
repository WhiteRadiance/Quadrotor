#include "capture.h"
#include "led.h"
#include "usart.h"
#include "sys.h"
//��һ��������:����pwm.c�� PWM�������
//�ڶ���������:���������  ���벶����


	//PWM�����ʼ��		//arr���Զ���װֵ		//psc��ʱ��Ԥ��Ƶ��
void TIM1_PWM_init(u16 arr,u16 psc)
{	//3����ʼ���ṹ��
	GPIO_InitTypeDef 					GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  	TIM_TimeBaseStructure;
	TIM_OCInitTypeDef 			 	TIM_OCInitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);//ʹ�ܶ�ʱ��1��ʱ��
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);//ʹ��GPIOA����ʱ��ʹ��
	                                                                     	

  //���ø�����Ϊ�����������,���TIM1 CH1(PA8����)��PWM���岨��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;//TIM_CH1(PA8����)
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//�����������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	
	TIM_TimeBaseStructure.TIM_Period = arr;//��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	 80K
	TIM_TimeBaseStructure.TIM_Prescaler =psc;//����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ  ����Ƶ
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;//����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);//����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ

 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;//ѡ��ʱ��ģʽ:TIM�����ȵ���ģʽ2
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//�Ƚ����ʹ��
	TIM_OCInitStructure.TIM_Pulse = 0;//���ô�װ�벶��ȽϼĴ���������ֵ
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;//�������:��Ƚ�ֵƥ���,TIM�������Ϊ��
	TIM_OC1Init(TIM1, &TIM_OCInitStructure);//����TIM_OCInitStruct��ָ���Ĳ�����ʼ������TIMx��OC1:���ͨ��1

  TIM_CtrlPWMOutputs(TIM1,ENABLE);//MOE �����ʹ��	

	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);//CH1Ԥװ��ʹ��	 
	
	TIM_ARRPreloadConfig(TIM1, ENABLE);//ʹ��TIMx��ARR�ϵ�Ԥװ�ؼĴ���
	
	TIM_Cmd(TIM1, ENABLE);//ʹ��TIM1
}


//��ʱ��2ͨ��1���벶������
TIM_ICInitTypeDef  TIM2_ICInitStructure;
void TIM2_Cap_init(u16 arr,u16 psc)
{	 
	//TIM_ICInitTypeDef  TIM2_ICInitStructure;//*********************************************
	
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
 	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);//ʹ��TIM2ʱ��
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//ʹ��GPIOAʱ��
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0;//PA0 ���֮ǰ����  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;//PA0 ��������  
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA,GPIO_Pin_0);
	
	//��ʼ����ʱ��2 TIM2	 
	TIM_TimeBaseStructure.TIM_Period = arr;//�趨�������Զ���װֵ 
	TIM_TimeBaseStructure.TIM_Prescaler =psc;//Ԥ��Ƶ��   
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;//����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);//����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
  
	//��ʼ��TIM2���벶�����
	TIM2_ICInitStructure.TIM_Channel = TIM_Channel_1;//CC1S=01,ѡ�������,IC1ӳ�䵽TI1��
	TIM2_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;//�����ز���
	TIM2_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;//ӳ�䵽TI1��
	TIM2_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;//���������Ƶ,����Ƶ 
	TIM2_ICInitStructure.TIM_ICFilter = 0x00;//IC1F=0000,���������˲���,���˲�
	TIM_ICInit(TIM2, &TIM2_ICInitStructure);
	
	//�жϷ����ʼ��
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;//TIM2�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//��ռ���ȼ�2��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;//�����ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);//����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ��� 
	
	TIM_ITConfig(TIM2,TIM_IT_Update|TIM_IT_CC1,ENABLE);//��������ж� ,����CC1IE�����ж�	
																		//��Ϊ����ϳ�ʱ��ʱ���������������Ҫ��������ж�
  TIM_Cmd(TIM2,ENABLE );//ʹ�ܶ�ʱ��2
}


//��ʱ��5�жϷ������
//��һ�����벶�������ص�else���,����ִ�и����ж�,����ֲ���һ���½���
u8  TIM2CH1_CAPTURE_STA = 0;//���벶��״̬		    				
u16	TIM2CH1_CAPTURE_VAL;//���벶��ֵ
void TIM2_IRQHandler(void)
{ 

 	if((TIM2CH1_CAPTURE_STA & 0x80)==0)//��δ�ɹ�����	
	{	  
			if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)//���������ж�
			{	    
					if(TIM2CH1_CAPTURE_STA & 0x40)//�Ѿ����񵽸ߵ�ƽ��
					{
							if((TIM2CH1_CAPTURE_STA & 0x3F)==0x3F)//�ߵ�ƽ̫����,ǿ����Ϊ�ߵ�ƽ�Ѿ�����
							{
									TIM2CH1_CAPTURE_STA |= 0x80;//��ǳɹ�������һ��
									TIM2CH1_CAPTURE_VAL = 0xFFFF;
							}
							else
									TIM2CH1_CAPTURE_STA++;
					}	 
			}
			if(TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)//����1���������¼�,������������,Ҳ�������½���
			{	
					if(TIM2CH1_CAPTURE_STA & 0x40)//�Ƿ��Ѿ��������һ��������,����ǰ����ľ����½���
					{	  			
							TIM2CH1_CAPTURE_STA |= 0x80;//��ǳɹ�����һ�� �ߵ�ƽ����
							TIM2CH1_CAPTURE_VAL = TIM_GetCapture1(TIM2);//��ȡ����ֵ
							TIM_OC1PolarityConfig(TIM2,TIM_ICPolarity_Rising);//CC1P=0,����Ϊ�����ز���
					}
					else//��δ��ʼ,��һ�β���������
					{
							TIM2CH1_CAPTURE_STA = 0;//���
							TIM2CH1_CAPTURE_VAL = 0;
							TIM_SetCounter(TIM2,0);
							TIM2CH1_CAPTURE_STA |= 0x40;		//��ǲ�����������
							TIM_OC1PolarityConfig(TIM2,TIM_ICPolarity_Falling);//CC1P=1,����Ϊ�½��ز���
					}		    
			}			     	    					   
 	}
	TIM_ClearITPendingBit(TIM2, TIM_IT_CC1|TIM_IT_Update);//����жϱ�־λ
}
