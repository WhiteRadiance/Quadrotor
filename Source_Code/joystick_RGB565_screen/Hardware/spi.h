#ifndef _SPI_H
#define _SPI_H
#include "sys.h"

//#define SPI1_1Line_TxOnly_DMA	1	//�����һ: ����SPI1Ϊ����ֻд, ������SPI_Tx��DMA(DMA1_CH3)

void SPI1_Init(void);			 	//��ʼ��spi1
void SPI2_Init(void);			 	//��ʼ��spi2
void SPI1_SetSpeed(u8 SpeedSet);	//����SPI�ٶ�
u8 	SPI1_ReadWriteByte(u8 TxData);	//SPI1���߶�дһ���ֽ�
u8 	SPI2_ReadWriteByte(u8 TxData);	//SPI2���߶�дһ���ֽ�
		 
#endif

