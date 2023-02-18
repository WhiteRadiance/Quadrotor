#include "timer.h"

/*
/	** AHB��SYSCLK����1��ƵΪ72MHz, APB2��AHB����1��ƵΪ72MHz, APB1��AHB2����2��ƵΪ36MHz **
/	** ��APB2����AHB��1��Ƶ, ��TIM1/8������2��Ƶ, Ҳ��APB2=18MHzʱTIM1������ʱ��Ϊ18*2=36MHz **
/	** ��APB1����AHB��1��Ƶ, ��TIM2-7������2��Ƶ, Ҳ��APB1=36MHzʱTIM2������ʱ��Ϊ36*2=72MHz **
/	TIM1, TIM8 ��ʱ����APB2ͨ��һ���ɼ��Ƶ��, ��ΪAPB2û�з�Ƶ, ����TIM1/8Ҳ�����Ƶ, ����72MHz
/	TIM2-7 ��ʱ����APB1+��Ƶ��, ��ΪAPB1��AHB������2��Ƶ, ����TIM2-7ʵ��ʱ���Ƿ�Ƶ���2��Ƶ, ��72MHz
*/
						 
//PWM�����ʼ��
//arr���Զ���װֵ
//psc��ʱ��Ԥ��Ƶ��
void TIM4_PWM_Init(u16 arr,u16 psc)
{  
	GPIO_InitTypeDef		GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef	TIM_TimeBaseStructure;	//ʱ����� �ṹ��
	TIM_OCInitTypeDef		TIM_OCInitStructure;	//OutputCampare�ṹ��

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);	//APB1_TIM4ʱ��ʹ��
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//GPIOBʱ��ʹ��
	                                                                     	
   //���ø�����Ϊ�����������,���TIM4_CH1/2/3/4��PWM���岨��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;	//PA6/7/8/9
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//�����������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;	//Ӱ�칦��
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	TIM_TimeBaseStructure.TIM_Prescaler = psc;					//����TIMx��Ԥ��Ƶֵ, ������Ƶ��
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;	//TIM4 ���ϼ���ģʽ
	TIM_TimeBaseStructure.TIM_Period = arr;						//�����Զ���װ�ؼĴ���, �����������ֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//Ϊ��ʱ����ETRP�˲����ṩʱ��: TDTS = x * Tck_tim
	//TIM_TimeBaseStructure.TIM_RepetitionCounter = 0x00;		//Ĭ��
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;			//��ʱ��ģʽΪPWM1: ����ֵ < Ԥ��ֵ ʱ�����Ч��ƽ
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; 	//�Ƚ����ʹ��
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;//Ĭ��: �رջ������
	TIM_OCInitStructure.TIM_Pulse = 0; 								//��װ�벶��ȽϼĴ�����ֵ, �����ֵ(����Ϊ0)
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; 		//�������: �ߵ�ƽ��Ч
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;		//Ĭ��
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;	//Ĭ��
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;	//Ĭ��
	
	TIM_OC1Init(TIM4, &TIM_OCInitStructure);//ע����к���ֻ�ܳ�ʼ��CH1
	TIM_OC2Init(TIM4, &TIM_OCInitStructure);	//��ʼ��TIM_CH2
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);	//��ʼ��TIM_CH3
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);	//��ʼ��TIM_CH4
	
	
	//TIM_CtrlPWMOutputs(TIM1,ENABLE);					//MOE�����ʹ��(���ڸ߼���ʱ��)

	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);//ע����к���ֻ��ʹ��TIM_CH1��Ԥװ��
	TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);	//TIM_CH2Ԥװ��ʹ��
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);	//TIM_CH3Ԥװ��ʹ��
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);	//TIM_CH4Ԥװ��ʹ��
	
	TIM_ARRPreloadConfig(TIM4, ENABLE);					//ʹ��ARR(�Զ�����)��Ԥװ��
	
	TIM_Cmd(TIM4, ENABLE);
}


//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//����ʹ�õ��Ƕ�ʱ��3!

void TIM3_Config_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;		//timer_init �ṹ��
	NVIC_InitTypeDef         NVIC_InitStructure;		//NVIC_init �ṹ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);	//ʱ��ʹ��
	
	TIM_TimeBaseStructure.TIM_Prescaler = psc;					//����TIMx��Ԥ��Ƶֵ, ������Ƶ��
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;	//TIM���ϼ���ģʽ
	TIM_TimeBaseStructure.TIM_Period = arr;						//�����Զ���װ�ؼĴ���, �����������ֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//Ϊ��ʱ����ETRP�˲����ṩʱ��: TDTS = x * Tck_tim
	//TIM_TimeBaseStructure.TIM_RepetitionCounter = 0x00;  		//ͨ�ö�ʱ������Ҫ���ô˳�Ա����
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);		//ʹ��TIM3�ж�֮�е� �����ж�

	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;				//TIM3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//��ռ���ȼ�1��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//�����ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM3,ENABLE);							//����(ʹ��)��ʱ��
}




extern u8	Trig_LED_Scan;		// 4Hz
extern u8	Trig_ADC_Battery;	// 2Hz

//TIM3 �жϷ������
void TIM3_IRQHandler(void)
{
	static u16 ms250=0, ms500=0;
	
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)	//���ָ����TIM�жϷ������
	{
		ms250++;
		ms500++;

		if(ms250 >= 250)	// 4Hz
		{
			ms250 = 0;
			Trig_ADC_Battery = 1;
		}
		else ;
		if(ms500 >= 500)	// 2Hz
		{
			ms500 = 0;
			Trig_LED_Scan = 1;
		}
		else ;
	}
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);			//���TIM3���жϴ�����λ
}
