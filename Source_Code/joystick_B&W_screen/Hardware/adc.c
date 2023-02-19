#include "delay.h"
#include "adc.h"

//初始化ADC
//这里我们仅以规则通道为例
//我们默认将开启ADC1_通道 0,1,2,3,9
void  adc1_init(void)
{
	ADC_InitTypeDef ADC_InitStructure; 
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	//使能ADC1通道时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//使能GPIOB时钟
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);						//分频72M/6 = 12M, MAX=14M

	//PA0 为 Thrust ADC1_CH0)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;//模拟输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//PA1 为 Yaw (ADC1_CH1)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//PA2 为 Pitch (ADC1_CH2)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//PA3 为 Roll (ADC1_CH3)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//PB1 为 Battery (ADC1_CH9)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	ADC_DeInit(ADC1);//复位ADC1,将外设 ADC1 的全部寄存器重设为缺省值

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;   		//ADC工作模式:ADC1工作在独立模式
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;         		//ADC 多通道扫描模式
	ADC_InitStructure.ADC_ContinuousConvMode = /*DISABLE*/ENABLE;   		//ADC 连续转换
	ADC_InitStructure.ADC_ExternalTrigConv = /*ADC_ExternalTrigConv_T3_TRGO*/ADC_ExternalTrigConv_None;	//转换由软件而不是外部触发启动
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;		//ADC数据右对齐
	ADC_InitStructure.ADC_NbrOfChannel = 5;               		//顺序进行规则转换的ADC通道数为5
	ADC_Init(ADC1, &ADC_InitStructure);                  		//初始化外设ADC1的寄存器


	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5);//CH0的转换等级是1, ADC转换时间252Cycle=21us
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_239Cycles5);//CH1的转换等级是2
	ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_239Cycles5);//CH2的转换等级是3
	ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 4, ADC_SampleTime_239Cycles5);//CH3的转换等级是4
	ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 5, ADC_SampleTime_239Cycles5);//CH9的转换等级是5
	
	ADC_DMACmd(ADC1, ENABLE);		//使能指定的ADC1的DMA请求
	ADC_Cmd(ADC1,ENABLE);			//使能指定的ADC1
	ADC_ResetCalibration(ADC1);		//使能复位校准
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);		//开启AD校准
	while(ADC_GetCalibrationStatus(ADC1));
}


void adc_dma_start(void)
{
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);	//ADC1软件转换启动
	DMA_Cmd(DMA1_Channel1, ENABLE);			//使能DMA通道1
}



//对数据进行简单滤波
u16 adc_average(u16* val0, u16* val1, u16* val2, u16* val3)
{
	//
	return 0;
}



//将航模锂电池的电压换算成百分比(自定义)
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
	else						pct = 222;					//超过4.2V为异常, 固定为222%
	return pct;
}


