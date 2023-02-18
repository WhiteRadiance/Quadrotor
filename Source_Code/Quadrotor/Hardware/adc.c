#include "delay.h"
#include "adc.h"

//初始化ADC
//这里我们仅以规则通道为例
//我们默认将开启ADC1_通道 10
void adc1_init(void)
{
	ADC_InitTypeDef ADC_InitStructure; 
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	//使能ADC1通道时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	//使能GPIOC时钟
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);						//分频72M/6 = 12M, ADC最大频率为14M

	//PC0 为 Battery (ADC1_CH10)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;//模拟输入
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	ADC_DeInit(ADC1);//复位ADC1

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;   		//ADC工作模式:ADC1工作在独立模式
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;         		//ADC 单通道模式
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;   		//ADC 单次转换
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//转换由软件而不是外部触发启动
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;		//ADC数据右对齐
	ADC_InitStructure.ADC_NbrOfChannel = 1;               		//顺序进行规则转换的ADC通道数为1
	ADC_Init(ADC1, &ADC_InitStructure);                  		//初始化外设ADC1的寄存器

	//ADC转换时间252Cycle = 18us
	ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5);//CH10的转换等级是1

	
	//ADC_DMACmd(ADC1, ENABLE);		//使能指定的ADC1的DMA请求
	ADC_Cmd(ADC1,ENABLE);			//使能指定的ADC1	
	ADC_ResetCalibration(ADC1);		//使能复位校准  	 
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);		//开启AD校准 
	while(ADC_GetCalibrationStatus(ADC1));
}


u16 get_once_ADC()
{
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);			//ADC1软件转换启动
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));	//等待转换结束
	
	return ADC_GetConversionValue(ADC1);//返回上一次刚刚转换好的通道数据
}


void adc_dma_start(void)
{
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);	//ADC1软件转换启动
	DMA_Cmd(DMA1_Channel1, ENABLE);			//使能DMA通道1
}


/*

//用于多次获取ADC值,取平均,可以提高精准度
u16 get_adc_average(u8 channel,u8 repeat)
{
	u32 temp_val = 0;//因为先加后除,所以u16一定不够用,加着加着16位就不够了,最后再除个数
	u8 t;
	for(t=0;t<repeat;t++)
	{
		temp_val += get_once_ADC(channel);
		delay_ms(5);
	}
	return temp_val/repeat;
} 	 
*/
