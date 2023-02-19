#ifndef _I2C_H
#define _I2C_H
#include "sys.h"


#define I2C_GPIO_PORT	GPIOB
#define I2C_GPIO_CLK	RCC_APB2Periph_GPIOB
#define SCL_GPIO_PIN	GPIO_Pin_6
#define SDA_GPIO_PIN	GPIO_Pin_7

//SDA引脚所在CRH或CRL寄存器的偏移
#define SDA_OFFSET	log2_offset(SDA_GPIO_PIN)



/**
  * @brief  对Bytes求对数(以2为底)
  * @note	必须保证整除, 如输入512, 则返回9
  * @param  Bytes: 字节数
  * @retval 以2为底的指数值
  */
static u8 log2_offset(u16 Bytes)
{
	u8 count = 0;
	while(Bytes != 1){
		Bytes >>= 1;
		count++;
	}
	return(count);
}

//将SDA配置为输入模式
void SDA_IN(void)
{
	if(SDA_OFFSET > 7){										//CRH
		I2C_GPIO_PORT->CRH &= ~(0xF<<((SDA_OFFSET-8)*4));	//清0, 复位GPIO的模式
		I2C_GPIO_PORT->CRH |= (4<<(SDA_OFFSET-8)*4);		//浮空输入
	/*	I2C_GPIO_PORT->CRH |= (8<<(SDA_OFFSET-8)*4);		//上拉/下拉输入
		I2C_GPIO_PORT->BSRR = SDA_GPIO_PIN;					//配置ODR为1则为上拉模式
		*/
	}
	else {													//CRL
		I2C_GPIO_PORT->CRL &= ~(0xF<<((SDA_OFFSET)*4));		//清0, 复位GPIO的模式
		I2C_GPIO_PORT->CRL |= (4<<(SDA_OFFSET)*4);			//浮空输入
	/*	I2C_GPIO_PORT->CRL |= (8<<(SDA_OFFSET)*4);			//上拉/下拉输入
		I2C_GPIO_PORT->BSRR = SDA_GPIO_PIN;					//配置ODR为1则为上拉模式
		*/
	}
}

//将SDA配置为输出模式
void SDA_OUT(void)
{
	if(SDA_OFFSET > 7){										//CRH
		I2C_GPIO_PORT->CRH &= ~(0xF<<((SDA_OFFSET-8)*4));	//清0, 复位GPIO的模式
		I2C_GPIO_PORT->CRH |= (5<<(SDA_OFFSET-8)*4);		//通用开漏输出, 10MHz
		//I2C_GPIO_PORT->CRH |= (7<<(SDA_OFFSET-8)*4);		//通用开漏输出, 50MHz
		//I2C_GPIO_PORT->CRH |= (1<<(SDA_OFFSET-8)*4);		//通用推挽输出, 10MHz
		//I2C_GPIO_PORT->CRH |= (3<<(SDA_OFFSET-8)*4);		//通用推挽输出, 50MHz
	}
	else {													//CRL
		I2C_GPIO_PORT->CRL &= ~(0xF<<((SDA_OFFSET)*4));		//清0, 复位GPIO的模式
		I2C_GPIO_PORT->CRL |= (5<<(SDA_OFFSET-8)*4);		//通用开漏输出, 10MHz
		//I2C_GPIO_PORT->CRL |= (7<<(SDA_OFFSET-8)*4);		//通用开漏输出, 50MHz
		//I2C_GPIO_PORT->CRL |= (1<<(SDA_OFFSET)*4);		//通用推挽输出, 10MHz
		//I2C_GPIO_PORT->CRL |= (3<<(SDA_OFFSET)*4);		//通用推挽输出, 50MHz
	}
}



#define SCL_H_1			I2C_GPIO_PORT->BSRR = SCL_GPIO_PIN	//BSRR寄存器 低16位写1则置1, 写0不变
#define SCL_L_0			I2C_GPIO_PORT->BRR  = SCL_GPIO_PIN	//BRR寄存器  低16位写1则置0, 写0不变
#define SDA_H_1			I2C_GPIO_PORT->BSRR = SDA_GPIO_PIN 
#define SDA_L_0			I2C_GPIO_PORT->BRR  = SDA_GPIO_PIN
#define READ_SDA		(((I2C_GPIO_PORT->IDR & SDA_GPIO_PIN) != 0) ? 1:0)	//读取输入寄存器的值


//IO操作函数	 
#define IIC_SCL    PCout(12) //SCL
#define IIC_SDA    PCout(11) //SDA	 
//#define READ_SDA   PCin(11)  //输入SDA 



//IIC所有操作函数
void IIC_Init(void);                //初始化IIC的IO口				 
void IIC_Start(void);				//发送IIC开始信号
void IIC_Stop(void);	  			//发送IIC停止信号
void IIC_Send_Byte(u8 txd);			//IIC发送一个字节
u8 IIC_Read_Byte(u8 ack);			//IIC读取一个字节
u8 IIC_Wait_Ack(void); 				//IIC等待ACK信号
void IIC_Ack(void);					//IIC发送ACK信号
void IIC_NAck(void);				//IIC不发送ACK信号




u8 I2C_WriteBytes(u8 ADDR, u8 REG, u8* Wr_BUF, u8 LEN);
u8 I2C_ReadBytes(u8 ADDR, u8 REG, u8* Rd_BUF, u8 LEN);


#endif

