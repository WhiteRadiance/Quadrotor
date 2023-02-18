#ifndef _I2C_H
#define _I2C_H
#include "sys.h"


#define I2C_GPIO_PORT	GPIOC
#define I2C_GPIO_CLK	RCC_APB2Periph_GPIOC
//#define SCL_GPIO_PIN	GPIO_Pin_12
//#define SDA_GPIO_PIN	GPIO_Pin_11
#define SCL_GPIO_PIN	GPIO_Pin_1
#define SDA_GPIO_PIN	GPIO_Pin_2



u8 log2_offset(u16 Bytes);//以2为底的对数



//SDA引脚所在CRH或CRL寄存器的偏移
#define SDA_OFFSET	log2_offset(SDA_GPIO_PIN)


#define SCL_H_1			I2C_GPIO_PORT->BSRR = SCL_GPIO_PIN	//BSRR寄存器 低16位写1则置1, 写0不变
#define SCL_L_0			I2C_GPIO_PORT->BRR  = SCL_GPIO_PIN	//BRR寄存器  低16位写1则置0, 写0不变
#define SDA_H_1			I2C_GPIO_PORT->BSRR = SDA_GPIO_PIN
#define SDA_L_0			I2C_GPIO_PORT->BRR  = SDA_GPIO_PIN
#define READ_SDA		(((I2C_GPIO_PORT->IDR & SDA_GPIO_PIN) != 0) ? 1:0)	//读取输入寄存器的值


//IO操作函数	 
//#define IIC_SCL    PCout(12) //SCL
//#define IIC_SDA    PCout(11) //SDA
#define IIC_SCL    PCout(1) //SCL
#define IIC_SDA    PCout(2) //SDA
//#define READ_SDA   PCin(2)  //输入SDA



//IIC所有操作函数
void IIC_Init(void);                //初始化IIC的IO口				 
void IIC_Start(void);				//发送IIC开始信号
void IIC_Stop(void);	  			//发送IIC停止信号
void IIC_Send_Byte(u8 txd);			//IIC发送一个字节
u8 IIC_Read_Byte(u8 ack);			//IIC读取一个字节
u8 IIC_Wait_Ack(void); 				//IIC等待ACK信号
void IIC_Ack(void);					//IIC发送ACK信号
void IIC_NAck(void);				//IIC不发送ACK信号



u8 I2C_WriteByte(u8 ADDR, u8 REG, u8 data);
u8 I2C_ReadByte(u8 ADDR, u8 REG, u8* pdata);

u8 I2C_WriteBytes(u8 ADDR, u8 REG, u8 LEN, u8* Wr_BUF);
u8 I2C_ReadBytes(u8 ADDR, u8 REG, u8 LEN, u8* Rd_BUF);


#endif

