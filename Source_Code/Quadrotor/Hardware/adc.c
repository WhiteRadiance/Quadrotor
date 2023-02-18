#include "delay.h"
#include "adc.h"

//��ʼ��ADC
//�������ǽ��Թ���ͨ��Ϊ��
//����Ĭ�Ͻ�����ADC1_ͨ�� 10
void adc1_init(void)
{
	ADC_InitTypeDef ADC_InitStructure; 
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	//ʹ��ADC1ͨ��ʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	//ʹ��GPIOCʱ��
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);						//��Ƶ72M/6 = 12M, ADC���Ƶ��Ϊ14M

	//PC0 Ϊ Battery (ADC1_CH10)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;//ģ������
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	ADC_DeInit(ADC1);//��λADC1

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;   		//ADC����ģʽ:ADC1�����ڶ���ģʽ
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;         		//ADC ��ͨ��ģʽ
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;   		//ADC ����ת��
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//ת��������������ⲿ��������
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;		//ADC�����Ҷ���
	ADC_InitStructure.ADC_NbrOfChannel = 1;               		//˳����й���ת����ADCͨ����Ϊ1
	ADC_Init(ADC1, &ADC_InitStructure);                  		//��ʼ������ADC1�ļĴ���

	//ADCת��ʱ��252Cycle = 18us
	ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5);//CH10��ת���ȼ���1

	
	//ADC_DMACmd(ADC1, ENABLE);		//ʹ��ָ����ADC1��DMA����
	ADC_Cmd(ADC1,ENABLE);			//ʹ��ָ����ADC1	
	ADC_ResetCalibration(ADC1);		//ʹ�ܸ�λУ׼  	 
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);		//����ADУ׼ 
	while(ADC_GetCalibrationStatus(ADC1));
}


u16 get_once_ADC()
{
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);			//ADC1���ת������
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));	//�ȴ�ת������
	
	return ADC_GetConversionValue(ADC1);//������һ�θո�ת���õ�ͨ������
}


void adc_dma_start(void)
{
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);	//ADC1���ת������
	DMA_Cmd(DMA1_Channel1, ENABLE);			//ʹ��DMAͨ��1
}


/*

//���ڶ�λ�ȡADCֵ,ȡƽ��,������߾�׼��
u16 get_adc_average(u8 channel,u8 repeat)
{
	u32 temp_val = 0;//��Ϊ�ȼӺ��,����u16һ��������,���ż���16λ�Ͳ�����,����ٳ�����
	u8 t;
	for(t=0;t<repeat;t++)
	{
		temp_val += get_once_ADC(channel);
		delay_ms(5);
	}
	return temp_val/repeat;
} 	 
*/
