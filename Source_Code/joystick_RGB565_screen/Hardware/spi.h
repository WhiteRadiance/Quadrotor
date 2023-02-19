#ifndef _SPI_H
#define _SPI_H
#include "sys.h"

//#define SPI1_1Line_TxOnly_DMA	1	//如果置一: 设置SPI1为单线只写, 并启用SPI_Tx的DMA(DMA1_CH3)

void SPI1_Init(void);			 	//初始化spi1
void SPI2_Init(void);			 	//初始化spi2
void SPI1_SetSpeed(u8 SpeedSet);	//设置SPI速度
u8 	SPI1_ReadWriteByte(u8 TxData);	//SPI1总线读写一个字节
u8 	SPI2_ReadWriteByte(u8 TxData);	//SPI2总线读写一个字节
		 
#endif

