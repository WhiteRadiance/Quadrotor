#include "timer.h"

/*
/	** AHB��SYSCLK����1��ƵΪ72MHz, APB2��AHB����1��ƵΪ72MHz, APB1��AHB2����2��ƵΪ36MHz **
/	** ��APB2����AHB��1��Ƶ, ��TIM1/8������2��Ƶ, Ҳ��APB2=18MHzʱTIM1������ʱ��Ϊ18*2=36MHz **
/	** ��APB1����AHB��1��Ƶ, ��TIM2-7������2��Ƶ, Ҳ��APB1=36MHzʱTIM1������ʱ��Ϊ36*2=72MHz **
/	TIM1, TIM8 ��ʱ����APB2ͨ��һ���ɼ��Ƶ��, ��ΪAPB2û�з�Ƶ, ����TIM1/8Ҳ�����Ƶ, ����72MHz
/	TIM2-7 ��ʱ����APB1+��Ƶ��, ��ΪAPB1��AHB������2��Ƶ, ����TIM2-7ʵ��ʱ���Ƿ�Ƶ���2��Ƶ, ��36MHz
*/
						 
//PWM�����ʼ��
//arr���Զ���װֵ
//psc��ʱ��Ԥ��Ƶ��
void TIM1_PWM_Init(u16 arr,u16 psc)
{  
	GPIO_InitTypeDef		GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef	TIM_TimeBaseStructure;	//ʱ����� �ṹ��
	TIM_OCInitTypeDef		TIM_OCInitStructure;	//OutputCampare�ṹ��

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);	//APB2_TIM1ʱ��ʹ��
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//ʹ��GPIO����ʱ��ʹ��
	                                                                     	
   //���ø�����Ϊ�����������,���TIM1_CH1��PWM���岨��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;			//P48
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//�����������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;	//Ӱ�칦��
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	TIM_TimeBaseStructure.TIM_Prescaler = psc;						//����TIMx�ļ������ļ���Ƶ��   9->7.2MHz
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;		//TIM���ϼ���ģʽ
	TIM_TimeBaseStructure.TIM_Period = arr;							//�����Զ���װ�ص�ֵ(��������)	 799->8kHz
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;			//Ϊ��ʱ����ETRP�˲����ṩʱ��: TDTS = x * Tck_tim
	//TIM_TimeBaseStructure.TIM_RepetitionCounter = 0x00;				//Ĭ��
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;				//ģʽΪPWM2: ����ֵ<Ԥ��ֵʱ�����Ч��ƽ
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; 	//�Ƚ����ʹ��
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;//Ĭ��: �رջ������
	TIM_OCInitStructure.TIM_Pulse = 0; 								//Ԥ��������ıȽ�ֵCCR1(ռ�ձ�Ϊ0%)
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; 		//�������: �ߵ�ƽ��Ч
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;		//Ĭ��
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;	//Ĭ��
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;	//Ĭ��
	TIM_OC1Init(TIM1, &TIM_OCInitStructure);//ע����к���ֻ�ܳ�ʼ��CH1
	
	
	TIM_CtrlPWMOutputs(TIM1,ENABLE);					//MOE�����ʹ��(���ڸ߼���ʱ��)

	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);	//CH1Ԥװ��ʹ��
	
	TIM_ARRPreloadConfig(TIM1, ENABLE);					//ʹ��ARR(�Զ�����)��Ԥװ��
	
	TIM_Cmd(TIM1, ENABLE);
}


//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//����ʹ�õ��Ƕ�ʱ��3!

void TIM3_Config_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef  	TIM_TimeBaseStructure;		//ʱ����� �ṹ��
	NVIC_InitTypeDef         	NVIC_InitStructure;			//NVIC_init �ṹ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);	//ʱ��ʹ��
	
	TIM_TimeBaseStructure.TIM_Prescaler = psc;					//����TIMx��Ԥ��Ƶֵ, ������Ƶ��
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;	//TIM���ϼ���ģʽ
	TIM_TimeBaseStructure.TIM_Period = arr;						//�����Զ���װ�ؼĴ���, �����������ֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//Ϊ��ʱ����ETRP�˲����ṩʱ��: TDTS = x * Tck_tim
	//TIM_TimeBaseStructure.TIM_RepetitionCounter = 0x00;  		//ͨ�ö�ʱ������Ҫ���ô˳�Ա����
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);			//ʹ��TIM3�ж�֮�е� �����ж�
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;				//TIM3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//��ռ���ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;			//�����ȼ�3��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);
	
	TIM_Cmd(TIM3, ENABLE);								//����(ʹ��)��ʱ��
}


