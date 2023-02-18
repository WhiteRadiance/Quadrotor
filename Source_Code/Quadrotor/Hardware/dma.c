#include "dma.h"

//u16 ADC_Mem[2];//对应于main里面的 "extern ADC_Mem[2]"

DMA_InitTypeDef  DMA_InitStructure;

//DMA1的各通道配置
//这里的传输形式是固定的,这点要根据不同的情况来修改
//从存储器->外设模式/8位数据宽度/存储器增量模式
//DMA_CHx:DMA通道CHx   //peripheral_addr:外设地址	 //memory_addr:存储器地址   //buffersize:数据传输量 
void dma_config(DMA_Channel_TypeDef* DMA_CHx, u32 peripheral_addr, u32 memory_addr,u16 buffersize)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);			//使能DMA传输
	DMA_DeInit(DMA_CHx);										//将DMA的通道1寄存器复位(默认值)

	DMA_InitStructure.DMA_PeripheralBaseAddr = peripheral_addr;	//DMA外设基地址
	DMA_InitStructure.DMA_MemoryBaseAddr = memory_addr;			//DMA内存基地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;			//数据传输方向为外设(ADC)至数组内存
	DMA_InitStructure.DMA_BufferSize = buffersize;						//DMA通道的DMA缓存的大小
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	//外设地址寄存器不自增
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;				//内存地址寄存器递增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	//数据宽度为16位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;			//数据宽度为16位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;				//工作在循环模式,循环采集
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;		//DMA通道x 的优先级为 中级 
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;				//DMA通道x没有设置为内存到内存传输
	DMA_Init(DMA_CHx, &DMA_InitStructure);						//根据DMA_InitStruct中指定的参数初始化DMA通道
}


//开启一次DMA传输,使能DMA1_CHx
void dma_enable(DMA_Channel_TypeDef* DMA_CHx)
{
 	DMA_Cmd(DMA_CHx, ENABLE);//使能USART1 TX DMA1 所指示的通道 
}

