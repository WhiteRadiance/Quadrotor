#ifndef _I2C_H
#define _I2C_H
#include "sys.h"


#define I2C_GPIO_PORT	GPIOC
#define I2C_GPIO_CLK	RCC_APB2Periph_GPIOC
//#define SCL_GPIO_PIN	GPIO_Pin_12
//#define SDA_GPIO_PIN	GPIO_Pin_11
#define SCL_GPIO_PIN	GPIO_Pin_1
#define SDA_GPIO_PIN	GPIO_Pin_2



u8 log2_offset(u16 Bytes);//��2Ϊ�׵Ķ���



//SDA��������CRH��CRL�Ĵ�����ƫ��
#define SDA_OFFSET	log2_offset(SDA_GPIO_PIN)


#define SCL_H_1			I2C_GPIO_PORT->BSRR = SCL_GPIO_PIN	//BSRR�Ĵ��� ��16λд1����1, д0����
#define SCL_L_0			I2C_GPIO_PORT->BRR  = SCL_GPIO_PIN	//BRR�Ĵ���  ��16λд1����0, д0����
#define SDA_H_1			I2C_GPIO_PORT->BSRR = SDA_GPIO_PIN
#define SDA_L_0			I2C_GPIO_PORT->BRR  = SDA_GPIO_PIN
#define READ_SDA		(((I2C_GPIO_PORT->IDR & SDA_GPIO_PIN) != 0) ? 1:0)	//��ȡ����Ĵ�����ֵ


//IO��������	 
//#define IIC_SCL    PCout(12) //SCL
//#define IIC_SDA    PCout(11) //SDA
#define IIC_SCL    PCout(1) //SCL
#define IIC_SDA    PCout(2) //SDA
//#define READ_SDA   PCin(2)  //����SDA



//IIC���в�������
void IIC_Init(void);                //��ʼ��IIC��IO��				 
void IIC_Start(void);				//����IIC��ʼ�ź�
void IIC_Stop(void);	  			//����IICֹͣ�ź�
void IIC_Send_Byte(u8 txd);			//IIC����һ���ֽ�
u8 IIC_Read_Byte(u8 ack);			//IIC��ȡһ���ֽ�
u8 IIC_Wait_Ack(void); 				//IIC�ȴ�ACK�ź�
void IIC_Ack(void);					//IIC����ACK�ź�
void IIC_NAck(void);				//IIC������ACK�ź�



u8 I2C_WriteByte(u8 ADDR, u8 REG, u8 data);
u8 I2C_ReadByte(u8 ADDR, u8 REG, u8* pdata);

u8 I2C_WriteBytes(u8 ADDR, u8 REG, u8 LEN, u8* Wr_BUF);
u8 I2C_ReadBytes(u8 ADDR, u8 REG, u8 LEN, u8* Rd_BUF);


#endif

