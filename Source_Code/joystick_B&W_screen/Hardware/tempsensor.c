#include "delay.h"
#include "adc.h"

//��ʼ��ADC
//�������ǽ��Թ���ͨ��Ϊ��
//����Ĭ�Ͻ�����ADC1_ͨ��0~3																	   
void T_adc_init(void)
{ 	
	ADC_InitTypeDef ADC_InitStructure; 

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1,ENABLE);//ʹ��GPIOA,ADC1ͨ��ʱ��
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);//����ADC��Ƶ����6 72M/6=12M,ADC���ʱ�䲻�ܳ���14M

	ADC_DeInit(ADC1);//��λADC1,������ ADC1 ��ȫ���Ĵ�������Ϊȱʡֵ

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;    //ADC����ģʽ:ADC1��ADC2�����ڶ���ģʽ
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;         //�����ڵ�ͨ��ģʽ
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;   //�����ڵ���ת��ģʽ
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;//ת��������������ⲿ��������
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;//ADC�����Ҷ���
	ADC_InitStructure.ADC_NbrOfChannel = 1;               //˳����й���ת����ADCͨ������ĿΪ1
	ADC_Init(ADC1, &ADC_InitStructure);                  //��ʼ������ADC1�ļĴ���   
 
	ADC_TempSensorVrefintCmd(ENABLE); //�����ڲ��¶ȴ�����...................................................................
	
	ADC_Cmd(ADC1,ENABLE);//ʹ��ָ����ADC1
	
	ADC_ResetCalibration(ADC1);//ʹ�ܸ�λУ׼  	 
	while(ADC_GetResetCalibrationStatus(ADC1));//�ȴ���λУ׼����
	
	ADC_StartCalibration(ADC1);//����ADУ׼ 
	while(ADC_GetCalibrationStatus(ADC1));//�ȴ�У׼����
}


//���ADCֵ
//channel:ͨ��ֵ 16_�ڲ��¶ȴ�����
u16 T_get_adc(u8 channel)   
{
  //����ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
	ADC_RegularChannelConfig(ADC1,ADC_Channel_16/*��ADC_Channel_tempsensor*/,1,ADC_SampleTime_239Cycles5);//ADC1,ADCͨ��,����ʱ��Ϊ239.5����,12M��һ�β�����ʱ21us��1Ϊrank,ֵ��1~16֮��,����ת�������Ÿߵ�λ,��DMAӦ��ʱ��ͨ��1rank=2���ӦADCͨ��1��ת�����������ADCDMA[1]��		    
  
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);//ʹ��ָ����ADC1�����ת����������	
	while( !ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC) );//�ȴ�ת������

	return ADC_GetConversionValue(ADC1);//�������һ��ADC1�������ת�����
}


//����times�λ�ȡADCֵ,ȡƽ��,������߾�׼��
u16 T_get_adc_average(u8 channel,u8 times)
{
	u32 temp_val = 0;//��Ϊ�ȼӺ��,����u16һ��������,���ż���16λ�Ͳ�����,����ٳ�����
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val += T_get_adc(channel);//TempSensor
		delay_ms(5);
	}
	return temp_val/times;
} 	 

