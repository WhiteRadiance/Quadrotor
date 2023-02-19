#include "delay.h"
#include "adc.h"

//初始化ADC
//这里我们仅以规则通道为例
//我们默认将开启ADC1_通道0~3																	   
void T_adc_init(void)
{ 	
	ADC_InitTypeDef ADC_InitStructure; 

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1,ENABLE);//使能GPIOA,ADC1通道时钟
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);//设置ADC分频因子6 72M/6=12M,ADC最大时间不能超过14M

	ADC_DeInit(ADC1);//复位ADC1,将外设 ADC1 的全部寄存器重设为缺省值

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;    //ADC工作模式:ADC1和ADC2工作在独立模式
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;         //工作在单通道模式
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;   //工作在单次转换模式
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;//转换由软件而不是外部触发启动
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;//ADC数据右对齐
	ADC_InitStructure.ADC_NbrOfChannel = 1;               //顺序进行规则转换的ADC通道的数目为1
	ADC_Init(ADC1, &ADC_InitStructure);                  //初始化外设ADC1的寄存器   
 
	ADC_TempSensorVrefintCmd(ENABLE); //开启内部温度传感器...................................................................
	
	ADC_Cmd(ADC1,ENABLE);//使能指定的ADC1
	
	ADC_ResetCalibration(ADC1);//使能复位校准  	 
	while(ADC_GetResetCalibrationStatus(ADC1));//等待复位校准结束
	
	ADC_StartCalibration(ADC1);//开启AD校准 
	while(ADC_GetCalibrationStatus(ADC1));//等待校准结束
}


//获得ADC值
//channel:通道值 16_内部温度传感器
u16 T_get_adc(u8 channel)   
{
  //设置指定ADC的规则组通道，一个序列，采样时间
	ADC_RegularChannelConfig(ADC1,ADC_Channel_16/*即ADC_Channel_tempsensor*/,1,ADC_SampleTime_239Cycles5);//ADC1,ADC通道,采样时间为239.5周期,12M下一次采样用时21us。1为rank,值在1~16之间,代表转换结果存放高低位,如DMA应用时若通道1rank=2则对应ADC通道1的转换结果保存于ADCDMA[1]。		    
  
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);//使能指定的ADC1的软件转换启动功能	
	while( !ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC) );//等待转换结束

	return ADC_GetConversionValue(ADC1);//返回最近一次ADC1规则组的转换结果
}


//用于times次获取ADC值,取平均,可以提高精准度
u16 T_get_adc_average(u8 channel,u8 times)
{
	u32 temp_val = 0;//因为先加后除,所以u16一定不够用,加着加着16位就不够了,最后再除个数
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val += T_get_adc(channel);//TempSensor
		delay_ms(5);
	}
	return temp_val/times;
} 	 

