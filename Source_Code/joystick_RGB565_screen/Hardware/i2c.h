#ifndef _I2C_H
#define _I2C_H
#include "sys.h"


#define I2C_GPIO_PORT	GPIOB
#define I2C_GPIO_CLK	RCC_APB2Periph_GPIOB
#define SCL_GPIO_PIN	GPIO_Pin_6
#define SDA_GPIO_PIN	GPIO_Pin_7

//SDA��������CRH��CRL�Ĵ�����ƫ��
#define SDA_OFFSET	log2_offset(SDA_GPIO_PIN)



/**
  * @brief  ��Bytes�����(��2Ϊ��)
  * @note	���뱣֤����, ������512, �򷵻�9
  * @param  Bytes: �ֽ���
  * @retval ��2Ϊ�׵�ָ��ֵ
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

//��SDA����Ϊ����ģʽ
void SDA_IN(void)
{
	if(SDA_OFFSET > 7){										//CRH
		I2C_GPIO_PORT->CRH &= ~(0xF<<((SDA_OFFSET-8)*4));	//��0, ��λGPIO��ģʽ
		I2C_GPIO_PORT->CRH |= (4<<(SDA_OFFSET-8)*4);		//��������
	/*	I2C_GPIO_PORT->CRH |= (8<<(SDA_OFFSET-8)*4);		//����/��������
		I2C_GPIO_PORT->BSRR = SDA_GPIO_PIN;					//����ODRΪ1��Ϊ����ģʽ
		*/
	}
	else {													//CRL
		I2C_GPIO_PORT->CRL &= ~(0xF<<((SDA_OFFSET)*4));		//��0, ��λGPIO��ģʽ
		I2C_GPIO_PORT->CRL |= (4<<(SDA_OFFSET)*4);			//��������
	/*	I2C_GPIO_PORT->CRL |= (8<<(SDA_OFFSET)*4);			//����/��������
		I2C_GPIO_PORT->BSRR = SDA_GPIO_PIN;					//����ODRΪ1��Ϊ����ģʽ
		*/
	}
}

//��SDA����Ϊ���ģʽ
void SDA_OUT(void)
{
	if(SDA_OFFSET > 7){										//CRH
		I2C_GPIO_PORT->CRH &= ~(0xF<<((SDA_OFFSET-8)*4));	//��0, ��λGPIO��ģʽ
		I2C_GPIO_PORT->CRH |= (5<<(SDA_OFFSET-8)*4);		//ͨ�ÿ�©���, 10MHz
		//I2C_GPIO_PORT->CRH |= (7<<(SDA_OFFSET-8)*4);		//ͨ�ÿ�©���, 50MHz
		//I2C_GPIO_PORT->CRH |= (1<<(SDA_OFFSET-8)*4);		//ͨ���������, 10MHz
		//I2C_GPIO_PORT->CRH |= (3<<(SDA_OFFSET-8)*4);		//ͨ���������, 50MHz
	}
	else {													//CRL
		I2C_GPIO_PORT->CRL &= ~(0xF<<((SDA_OFFSET)*4));		//��0, ��λGPIO��ģʽ
		I2C_GPIO_PORT->CRL |= (5<<(SDA_OFFSET-8)*4);		//ͨ�ÿ�©���, 10MHz
		//I2C_GPIO_PORT->CRL |= (7<<(SDA_OFFSET-8)*4);		//ͨ�ÿ�©���, 50MHz
		//I2C_GPIO_PORT->CRL |= (1<<(SDA_OFFSET)*4);		//ͨ���������, 10MHz
		//I2C_GPIO_PORT->CRL |= (3<<(SDA_OFFSET)*4);		//ͨ���������, 50MHz
	}
}



#define SCL_H_1			I2C_GPIO_PORT->BSRR = SCL_GPIO_PIN	//BSRR�Ĵ��� ��16λд1����1, д0����
#define SCL_L_0			I2C_GPIO_PORT->BRR  = SCL_GPIO_PIN	//BRR�Ĵ���  ��16λд1����0, д0����
#define SDA_H_1			I2C_GPIO_PORT->BSRR = SDA_GPIO_PIN 
#define SDA_L_0			I2C_GPIO_PORT->BRR  = SDA_GPIO_PIN
#define READ_SDA		(((I2C_GPIO_PORT->IDR & SDA_GPIO_PIN) != 0) ? 1:0)	//��ȡ����Ĵ�����ֵ


//IO��������	 
#define IIC_SCL    PCout(12) //SCL
#define IIC_SDA    PCout(11) //SDA	 
//#define READ_SDA   PCin(11)  //����SDA 



//IIC���в�������
void IIC_Init(void);                //��ʼ��IIC��IO��				 
void IIC_Start(void);				//����IIC��ʼ�ź�
void IIC_Stop(void);	  			//����IICֹͣ�ź�
void IIC_Send_Byte(u8 txd);			//IIC����һ���ֽ�
u8 IIC_Read_Byte(u8 ack);			//IIC��ȡһ���ֽ�
u8 IIC_Wait_Ack(void); 				//IIC�ȴ�ACK�ź�
void IIC_Ack(void);					//IIC����ACK�ź�
void IIC_NAck(void);				//IIC������ACK�ź�




u8 I2C_WriteBytes(u8 ADDR, u8 REG, u8* Wr_BUF, u8 LEN);
u8 I2C_ReadBytes(u8 ADDR, u8 REG, u8* Rd_BUF, u8 LEN);


#endif

