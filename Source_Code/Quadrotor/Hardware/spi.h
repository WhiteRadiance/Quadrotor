#ifndef __SPI_H
#define __SPI_H
#include "sys.h"
  	    													  
void SPI1_Init(void);			 	//初始化spi1
void SPI2_Init(void);			 	//初始化spi2
void SPI1_SetSpeed(u8 SpeedSet);	//设置SPI速度   
u8 	SPI1_ReadWriteByte(u8 TxData);	//SPI1总线读写一个字节
u8 	SPI2_ReadWriteByte(u8 TxData);	//SPI2总线读写一个字节
		 
#endif

