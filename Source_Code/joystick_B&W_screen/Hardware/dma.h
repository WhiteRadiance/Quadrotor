#ifndef __DMA_H
#define	__DMA_H	   

#include "stm32f10x.h"							    					    
								//DMAͨ��	 	/�����ַ		/�洢����ַ 	/����������
void dma_config(DMA_Channel_TypeDef* DMA_CHx, u32 periph_addr, u32 mem_addr, u16 buffsize);//����DMA1_CHx
void dma_enable(DMA_Channel_TypeDef* DMA_CHx);//ʹ��DMA1_CHx
		   
#endif
