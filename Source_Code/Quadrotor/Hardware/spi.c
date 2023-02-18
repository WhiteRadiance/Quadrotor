#include "spi.h"
//������SPIģ��ĳ�ʼ������, ���ó���ģʽ
//����SD Card /W25X16 /NRF24L01, ���ǵ�CS�ֱ���SD_CS(PA3), F_CS(PA2), NRF_CS(PC4)

// spi1_sck(PA5),	spi1_miso(PA6), 	spi1_mosi(PA7)
// spi2_sck(PB13), 	spi2_miso(PB14), 	spi2_mosi(PB15)
// spi3_sck(PB3), 	spi3_miso(PB4), 	spi3_mosi(PB5)

//�������Ƕ�SPI1�ĳ�ʼ��
SPI_InitTypeDef  SPI_InitStructure;

void SPI1_Init(void)//spi1_sclk(PA5),spi1_miso(PA6),spi1_mosi(PA7),��CSʹ���������
{
	GPIO_InitTypeDef GPIO_InitStructure;
  
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE );	
 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;	//PA.567 --> spi1
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  					//�����������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

 	GPIO_ResetBits(GPIOA,GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7);			//��ʼ��ֵ�͵�ƽ

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;	//����SPIΪ˫��˫��ȫ˫��
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;						//����SPIΪ��ģʽ
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//����SPI���ͽ���8λ֡�ṹ
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;				//����SCK����ʱΪ��
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;			//�ڵ�һ��ʱ���ز���
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;							//����NSS�ź����������
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;	//����SPI������ 18M, SPI1=72M/4,SPI23=36/4,ע��Vmax=18M
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;					//ָ�����ݴ����MSBλ��ʼ
	SPI_InitStructure.SPI_CRCPolynomial = 5;				//CRCУ�����ʽ(>=1����)
	SPI_Init(SPI1, &SPI_InitStructure);  					//����SPI_InitStruct��ָ���Ĳ�����ʼ��SPIx
 
	SPI_Cmd(SPI1, ENABLE); 									//ʹ��SPI����
	
	//SPI1_ReadWriteByte(0xff);//��������		 
}

//SPI �ٶ����ú���(spi����ٶ�18M)
//SPI_BaudRatePrescaler_8   8��Ƶ   (SPI 9M   @sys APB1-72M, APB2-36M)
//SPI_BaudRatePrescaler_16  16��Ƶ  (SPI 4.5M @sys APB1-72M, APB2-36M)
//SPI_BaudRatePrescaler_256 256��Ƶ (SPI 281K @sys APB1-72M, APB2-36M)
void SPI1_SetSpeed(u8 SpeedSet)
{
	SPI_InitStructure.SPI_BaudRatePrescaler = SpeedSet ;
  	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_Cmd(SPI1,ENABLE);
} 

//SPIx ��дһ���ֽ�
//TxData:Ҫд����ֽ�
//����ֵ:��ȡ�����ֽ�
u8 SPI1_ReadWriteByte(u8 TxData)
{		
	u8 retry=0;				 	
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)	//���ͻ���ձ�־λ
	{
		retry++;
		if(retry>200) return 0;
	}			  
	SPI_I2S_SendData(SPI1, TxData);		//ͨ������SPIx����һ������
	retry=0;

	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)	//���ܻ���ǿձ�־λ
	{
		retry++;
		if(retry>200) return 0;
	}	  						    
	return SPI_I2S_ReceiveData(SPI1); 	//����ͨ��SPIx������յ�����
}




void SPI2_Init(void)//spi2_sclk(PB13),spi2_miso(PB14),spi2_mosi(PB15),��CSʹ���������
{
	GPIO_InitTypeDef GPIO_InitStructure;
  
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE );
//	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOB, ENABLE );	//APB2_Clock
//	RCC_APB1PeriphClockCmd(	RCC_APB1Periph_SPI2, ENABLE );	//APB1_Clock	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 |GPIO_Pin_6 |GPIO_Pin_7;
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 |GPIO_Pin_14 |GPIO_Pin_15;	//PB.13/14/15 --> spi2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  						//�����������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
//	GPIO_Init(GPIOB, &GPIO_InitStructure);

 	GPIO_ResetBits(GPIOA, GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7);
//	GPIO_ResetBits(GPIOB, GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);			//��ʼ��ֵ�͵�ƽ

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;	//����SPIΪ˫��˫��ȫ˫��
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;						//����SPIΪ��ģʽ
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//����SPI���ͽ���8λ֡�ṹ
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;				//����SCK����ʱΪ��
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;			//�ڵ�һ��ʱ���ز���
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;							//����NSS�ź����������
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;	//����SPI������ 9M, SPI23=36M/8,SPI1=72M/8,ע��Vmax=18M
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;					//ָ�����ݴ����MSBλ��ʼ
	SPI_InitStructure.SPI_CRCPolynomial = 5;				//CRCУ�����ʽ(>=1����)
	SPI_Init(SPI1, &SPI_InitStructure);
//	SPI_Init(SPI2, &SPI_InitStructure);  					//����SPI_InitStruct��ָ���Ĳ�����ʼ��SPIx

	SPI_Cmd(SPI1, ENABLE);
//	SPI_Cmd(SPI2, ENABLE); 									//ʹ��SPI����
	
	//SPI1_ReadWriteByte(0xff);//��������		 
}
u8 SPI2_ReadWriteByte(u8 TxData)
{		
	u8 retry=0;				 	
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)	//���ͻ���ձ�־λ
	{
		retry++;
		if(retry>200) return 0;
	}			  
	SPI_I2S_SendData(SPI1, TxData);		//ͨ������SPIx����һ������
	retry=0;

	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)	//���ܻ���ǿձ�־λ
	{
		retry++;
		if(retry>200) return 0;
	}	  						    
	return SPI_I2S_ReceiveData(SPI1); 	//����ͨ��SPIx������յ�����
}

