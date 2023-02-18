#include "dma.h"

//u16 ADC_Mem[2];//��Ӧ��main����� "extern ADC_Mem[2]"

DMA_InitTypeDef  DMA_InitStructure;

//DMA1�ĸ�ͨ������
//����Ĵ�����ʽ�ǹ̶���,���Ҫ���ݲ�ͬ��������޸�
//�Ӵ洢��->����ģʽ/8λ���ݿ��/�洢������ģʽ
//DMA_CHx:DMAͨ��CHx   //peripheral_addr:�����ַ	 //memory_addr:�洢����ַ   //buffersize:���ݴ����� 
void dma_config(DMA_Channel_TypeDef* DMA_CHx, u32 peripheral_addr, u32 memory_addr,u16 buffersize)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);			//ʹ��DMA����
	DMA_DeInit(DMA_CHx);										//��DMA��ͨ��1�Ĵ�����λ(Ĭ��ֵ)

	DMA_InitStructure.DMA_PeripheralBaseAddr = peripheral_addr;	//DMA�������ַ
	DMA_InitStructure.DMA_MemoryBaseAddr = memory_addr;			//DMA�ڴ����ַ
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;			//���ݴ��䷽��Ϊ����(ADC)�������ڴ�
	DMA_InitStructure.DMA_BufferSize = buffersize;						//DMAͨ����DMA����Ĵ�С
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	//�����ַ�Ĵ���������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;				//�ڴ��ַ�Ĵ�������
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	//���ݿ��Ϊ16λ
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;			//���ݿ��Ϊ16λ
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;				//������ѭ��ģʽ,ѭ���ɼ�
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;		//DMAͨ��x �����ȼ�Ϊ �м� 
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;				//DMAͨ��xû������Ϊ�ڴ浽�ڴ洫��
	DMA_Init(DMA_CHx, &DMA_InitStructure);						//����DMA_InitStruct��ָ���Ĳ�����ʼ��DMAͨ��
}


//����һ��DMA����,ʹ��DMA1_CHx
void dma_enable(DMA_Channel_TypeDef* DMA_CHx)
{
 	DMA_Cmd(DMA_CHx, ENABLE);//ʹ��USART1 TX DMA1 ��ָʾ��ͨ�� 
}

