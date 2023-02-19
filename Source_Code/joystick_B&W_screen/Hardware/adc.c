#include "delay.h"
#include "adc.h"

//��ʼ��ADC
//�������ǽ��Թ���ͨ��Ϊ��
//����Ĭ�Ͻ�����ADC1_ͨ�� 0,1,2,3,9
void  adc1_init(void)
{
	ADC_InitTypeDef ADC_InitStructure; 
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	//ʹ��ADC1ͨ��ʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//ʹ��GPIOAʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//ʹ��GPIOBʱ��
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);						//��Ƶ72M/6 = 12M, MAX=14M

	//PA0 Ϊ Thrust ADC1_CH0)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;//ģ������
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//PA1 Ϊ Yaw (ADC1_CH1)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//PA2 Ϊ Pitch (ADC1_CH2)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//PA3 Ϊ Roll (ADC1_CH3)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//PB1 Ϊ Battery (ADC1_CH9)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	ADC_DeInit(ADC1);//��λADC1,������ ADC1 ��ȫ���Ĵ�������Ϊȱʡֵ

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;   		//ADC����ģʽ:ADC1�����ڶ���ģʽ
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;         		//ADC ��ͨ��ɨ��ģʽ
	ADC_InitStructure.ADC_ContinuousConvMode = /*DISABLE*/ENABLE;   		//ADC ����ת��
	ADC_InitStructure.ADC_ExternalTrigConv = /*ADC_ExternalTrigConv_T3_TRGO*/ADC_ExternalTrigConv_None;	//ת��������������ⲿ��������
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;		//ADC�����Ҷ���
	ADC_InitStructure.ADC_NbrOfChannel = 5;               		//˳����й���ת����ADCͨ����Ϊ5
	ADC_Init(ADC1, &ADC_InitStructure);                  		//��ʼ������ADC1�ļĴ���


	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5);//CH0��ת���ȼ���1, ADCת��ʱ��252Cycle=21us
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_239Cycles5);//CH1��ת���ȼ���2
	ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_239Cycles5);//CH2��ת���ȼ���3
	ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 4, ADC_SampleTime_239Cycles5);//CH3��ת���ȼ���4
	ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 5, ADC_SampleTime_239Cycles5);//CH9��ת���ȼ���5
	
	ADC_DMACmd(ADC1, ENABLE);		//ʹ��ָ����ADC1��DMA����
	ADC_Cmd(ADC1,ENABLE);			//ʹ��ָ����ADC1
	ADC_ResetCalibration(ADC1);		//ʹ�ܸ�λУ׼
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);		//����ADУ׼
	while(ADC_GetCalibrationStatus(ADC1));
}


void adc_dma_start(void)
{
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);	//ADC1���ת������
	DMA_Cmd(DMA1_Channel1, ENABLE);			//ʹ��DMAͨ��1
}



//�����ݽ��м��˲�
u16 adc_average(u16* val0, u16* val1, u16* val2, u16* val3)
{
	//
	return 0;
}



//����ģ﮵�صĵ�ѹ����ɰٷֱ�(�Զ���)
u8 Lithium_Battery_percent(float volt)
{
	u8 pct = 0;
	if(volt <= 3.5)				pct = 0;					//3.5V ->  0%
	else if(volt <= 3.6)		pct = (volt-3.5)*50;		//3.6V ->  5%
	else if(volt <= 3.7)		pct = (volt-3.6)*100 + 5;	//3.7V -> 15%
	else if(volt <= 3.8)		pct = (volt-3.7)*250 + 15;	//3.8V -> 40%
	else if(volt <= 3.9)		pct = (volt-3.8)*250 + 40;	//3.9V -> 65%
	else if(volt <= 4.0)		pct = (volt-3.9)*250 + 65;	//4.0V -> 90%
	else if(volt <= 4.1)		pct = (volt-4.0)*70 + 90;	//4.1V -> 97%
	else if(volt <= 4.2)		pct = (volt-4.1)*30 + 97;	//4.2V ->100%
	else						pct = 222;					//����4.2VΪ�쳣, �̶�Ϊ222%
	return pct;
}


