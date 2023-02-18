#include "spi.h"
//以下是SPI模块的初始化代码, 配置成主模式
//访问SD Card /W25X16 /NRF24L01, 它们的CS分别在SD_CS(PA3), F_CS(PA2), NRF_CS(PC4)

// spi1_sck(PA5),	spi1_miso(PA6), 	spi1_mosi(PA7)
// spi2_sck(PB13), 	spi2_miso(PB14), 	spi2_mosi(PB15)
// spi3_sck(PB3), 	spi3_miso(PB4), 	spi3_mosi(PB5)

//这里针是对SPI1的初始化
SPI_InitTypeDef  SPI_InitStructure;

void SPI1_Init(void)//spi1_sclk(PA5),spi1_miso(PA6),spi1_mosi(PA7),其CS使用软件管理
{
	GPIO_InitTypeDef GPIO_InitStructure;
  
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE );	
 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;	//PA.567 --> spi1
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  					//复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

 	GPIO_ResetBits(GPIOA,GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7);			//初始赋值低电平

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;	//设置SPI为双线双向全双工
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;						//设置SPI为主模式
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//设置SPI发送接收8位帧结构
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;				//设置SCK空闲时为低
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;			//在第一个时钟沿采样
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;							//设置NSS信号由软件控制
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;	//设置SPI波特率 18M, SPI1=72M/4,SPI23=36/4,注意Vmax=18M
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;					//指定数据传输从MSB位开始
	SPI_InitStructure.SPI_CRCPolynomial = 5;				//CRC校验多项式(>=1就行)
	SPI_Init(SPI1, &SPI_InitStructure);  					//根据SPI_InitStruct中指定的参数初始化SPIx
 
	SPI_Cmd(SPI1, ENABLE); 									//使能SPI外设
	
	//SPI1_ReadWriteByte(0xff);//启动传输		 
}

//SPI 速度设置函数(spi最大速度18M)
//SPI_BaudRatePrescaler_8   8分频   (SPI 9M   @sys APB1-72M, APB2-36M)
//SPI_BaudRatePrescaler_16  16分频  (SPI 4.5M @sys APB1-72M, APB2-36M)
//SPI_BaudRatePrescaler_256 256分频 (SPI 281K @sys APB1-72M, APB2-36M)
void SPI1_SetSpeed(u8 SpeedSet)
{
	SPI_InitStructure.SPI_BaudRatePrescaler = SpeedSet ;
  	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_Cmd(SPI1,ENABLE);
} 

//SPIx 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节
u8 SPI1_ReadWriteByte(u8 TxData)
{		
	u8 retry=0;				 	
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)	//发送缓存空标志位
	{
		retry++;
		if(retry>200) return 0;
	}			  
	SPI_I2S_SendData(SPI1, TxData);		//通过外设SPIx发送一个数据
	retry=0;

	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)	//接受缓存非空标志位
	{
		retry++;
		if(retry>200) return 0;
	}	  						    
	return SPI_I2S_ReceiveData(SPI1); 	//返回通过SPIx最近接收的数据
}




void SPI2_Init(void)//spi2_sclk(PB13),spi2_miso(PB14),spi2_mosi(PB15),其CS使用软件管理
{
	GPIO_InitTypeDef GPIO_InitStructure;
  
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE );
//	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOB, ENABLE );	//APB2_Clock
//	RCC_APB1PeriphClockCmd(	RCC_APB1Periph_SPI2, ENABLE );	//APB1_Clock	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 |GPIO_Pin_6 |GPIO_Pin_7;
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 |GPIO_Pin_14 |GPIO_Pin_15;	//PB.13/14/15 --> spi2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  						//复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
//	GPIO_Init(GPIOB, &GPIO_InitStructure);

 	GPIO_ResetBits(GPIOA, GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7);
//	GPIO_ResetBits(GPIOB, GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);			//初始赋值低电平

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;	//设置SPI为双线双向全双工
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;						//设置SPI为主模式
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//设置SPI发送接收8位帧结构
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;				//设置SCK空闲时为低
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;			//在第一个时钟沿采样
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;							//设置NSS信号由软件控制
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;	//设置SPI波特率 9M, SPI23=36M/8,SPI1=72M/8,注意Vmax=18M
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;					//指定数据传输从MSB位开始
	SPI_InitStructure.SPI_CRCPolynomial = 5;				//CRC校验多项式(>=1就行)
	SPI_Init(SPI1, &SPI_InitStructure);
//	SPI_Init(SPI2, &SPI_InitStructure);  					//根据SPI_InitStruct中指定的参数初始化SPIx

	SPI_Cmd(SPI1, ENABLE);
//	SPI_Cmd(SPI2, ENABLE); 									//使能SPI外设
	
	//SPI1_ReadWriteByte(0xff);//启动传输		 
}
u8 SPI2_ReadWriteByte(u8 TxData)
{		
	u8 retry=0;				 	
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)	//发送缓存空标志位
	{
		retry++;
		if(retry>200) return 0;
	}			  
	SPI_I2S_SendData(SPI1, TxData);		//通过外设SPIx发送一个数据
	retry=0;

	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)	//接受缓存非空标志位
	{
		retry++;
		if(retry>200) return 0;
	}	  						    
	return SPI_I2S_ReceiveData(SPI1); 	//返回通过SPIx最近接收的数据
}

