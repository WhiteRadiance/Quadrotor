#ifndef __DMA_H
#define	__DMA_H	   

#include "stm32f10x.h"							    					    
								//DMA通道	 	/外设地址		/存储器地址 	/传输数据量
void dma_config(DMA_Channel_TypeDef* DMA_CHx, u32 periph_addr, u32 mem_addr, u16 buffsize);//配置DMA1_CHx
void dma_enable(DMA_Channel_TypeDef* DMA_CHx);//使能DMA1_CHx
		   
#endif
