#include "i2c.h"
#include "delay.h"


/**
  * @brief  ��Bytes�����(��2Ϊ��)
  * @note	���뱣֤����, ������512, �򷵻�9
  * @param  Bytes: �ֽ���
  * @retval ��2Ϊ�׵�ָ��ֵ
  */
u8 log2_offset(u16 Bytes)
{
	u8 count = 0;
	while(Bytes != 1){
		Bytes >>= 1;
		count++;
	}
	return(count);
}
#if 0
//��SDA����Ϊ����ģʽ
static void SDA_IN(void)
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
static void SDA_OUT(void)
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
		I2C_GPIO_PORT->CRL |= (5<<(SDA_OFFSET)*4);			//ͨ�ÿ�©���, 10MHz
		//I2C_GPIO_PORT->CRL |= (7<<(SDA_OFFSET-8)*4);		//ͨ�ÿ�©���, 50MHz
		//I2C_GPIO_PORT->CRL |= (1<<(SDA_OFFSET)*4);		//ͨ���������, 10MHz
		//I2C_GPIO_PORT->CRL |= (3<<(SDA_OFFSET)*4);		//ͨ���������, 50MHz
	}
}
#endif

#define SDA_IN()  {GPIOC->CRL&=0XFFFFF0FF; GPIOC->CRL|=(u32)4<<8;}
#define SDA_OUT() {GPIOC->CRL&=0XFFFFF0FF; GPIOC->CRL|=(u32)5<<8;}


//��ʼ��IIC
void IIC_Init(void)
{					     
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(I2C_GPIO_CLK, ENABLE);	
	   
	GPIO_InitStructure.GPIO_Pin = SCL_GPIO_PIN | SDA_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;			//��©���
	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;			//�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(I2C_GPIO_PORT, &GPIO_InitStructure);

	//SCL_H_1;		//����ʱΪ�ߵ�ƽ
	//SDA_H_1;		//����ʱΪ�ߵ�ƽ
	IIC_SCL = 1;
	delay_us(1);
	IIC_SDA = 1;
	delay_us(1);
}


//����IIC��ʼ�ź�
void IIC_Start(void)
{
	SDA_OUT();     //sda�����
	//SCL_H_1;
	//SDA_H_1;
	IIC_SDA = 1;
	IIC_SCL = 1;
	delay_us(3);
 	IIC_SDA = 0;//START: when CLK is high,DATA change from high to low 
	delay_us(4);
	IIC_SCL = 0;//ǯסI2C���ߣ�׼�����ͻ��������
	delay_us(1);
}


//����IICֹͣ�ź�
void IIC_Stop(void)
{
	SDA_OUT();//sda�����
	IIC_SCL = 0;
	IIC_SDA = 0;//STOP: when CLK is high DATA change form low to high
 	delay_us(4);
	IIC_SCL = 1;
	delay_us(4);
	IIC_SDA = 1;//����I2C���߽����ź�
	delay_us(1);							   	
}


//�ȴ�Ӧ���źŵ���
//����ֵ��1������Ӧ��ʧ��
//        0������Ӧ��ɹ�
u8 IIC_Wait_Ack(void)
{
	u8 ErrTime = 0;
	SDA_IN();      //SDA����Ϊ����  
	IIC_SDA = 1; delay_us(1);	   
	IIC_SCL = 1; delay_us(1);	 
	while(READ_SDA)
	{
		ErrTime++;
		if(ErrTime>250)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL=0;//ʱ�����0 	   
	return 0;  
}


//����ACKӦ��
void IIC_Ack(void)
{
	IIC_SCL = 0;
	SDA_OUT();
	IIC_SDA = 0;
	delay_us(2);
	IIC_SCL = 1;
	delay_us(3);
	IIC_SCL = 0;
	delay_us(1);
	//IIC_SDA = 1;	//�ͷ�����
}


//������ACKӦ��		    
void IIC_NAck(void)
{
	IIC_SCL = 0;
	SDA_OUT();
	IIC_SDA = 1;
	delay_us(2);
	IIC_SCL = 1;
	delay_us(3);
	IIC_SCL = 0;
	//�����ͷ�������, ��Ϊ����stopͨ��
}


//IIC����һ���ֽ�
//���شӻ�����Ӧ��
//1����Ӧ��
//0����Ӧ��			  
void IIC_Send_Byte(u8 data)
{                        
    u8 t;   
	SDA_OUT(); 	    
    IIC_SCL=0;//����ʱ�ӿ�ʼ���ݴ���
	delay_us(1);
    for(t=0;t<8;t++)
    {
		if(data & 0x80)
			IIC_SDA = 1;
		else
			IIC_SDA = 0;
		delay_us(2);
		IIC_SCL = 1;//�����ݱ��ӻ�ȡ��
		delay_us(3);
		IIC_SCL = 0;
		delay_us(2);
		data <<= 1;
    }
	//IIC_SDA = 1;	//�ͷ�����	
}


//��1���ֽڣ� /ack=1ʱ������ACK��/ack=0������nACK   
u8 IIC_Read_Byte(u8 ack)
{
	u8 i, receive = 0;
	SDA_IN();		//SDA����Ϊ����
    for(i=0;i<8;i++ )
	{
        IIC_SCL = 0; 
        delay_us(3);
		IIC_SCL = 1;
		delay_us(2);
        receive <<= 1;
        if(READ_SDA) receive++;   
		else ;
		delay_us(1);
		
		IIC_SCL = 0; 
        delay_us(1);
    }					 
    if(ack)
        IIC_Ack();	//����ACK
    else
        IIC_NAck();	//����nACK   
    return receive;
}

/*------------------------------------------------------------------------------------- /
/                          �Ѿ���װ�õĵĶ�д������غ���                               /
/ ------------------------------------------------------------------------------------ */

//return 0 if OK, 1 if Failed.
u8 I2C_WriteByte(u8 ADDR, u8 REG, u8 data)
{
	IIC_Start();
	IIC_Send_Byte((ADDR << 1) + 0);//0: Write Direction
	if(IIC_Wait_Ack()) return 1;
	IIC_Send_Byte(REG);
	if(IIC_Wait_Ack()) return 1;
	IIC_Send_Byte(data);
	if(IIC_Wait_Ack()) return 1;
	IIC_Stop();
	return 0;
}

//return 0 if OK, 1 if Failed.
u8 I2C_ReadByte(u8 ADDR, u8 REG, u8* pdata)
{
	IIC_Start();
	IIC_Send_Byte((ADDR << 1) + 0);//0: Write Direction
	if(IIC_Wait_Ack()) return 1;
	IIC_Send_Byte(REG);
	if(IIC_Wait_Ack()) return 1;
	
	IIC_Start();//ReStart
	IIC_Send_Byte((ADDR << 1) + 1);//1: Read Direction
	if(IIC_Wait_Ack()) return 1;
	*pdata = IIC_Read_Byte(0);//with nACK
	IIC_Stop();
	return 0;
}

//return 0 means OK, 1 means Failed.
u8 I2C_WriteBytes(u8 ADDR, u8 REG, u8 LEN, u8* Wr_BUF)
{
	IIC_Start();
	IIC_Send_Byte((ADDR << 1) + 0);//0: Write Direction
	if(IIC_Wait_Ack()) return 1;
	IIC_Send_Byte(REG);
	if(IIC_Wait_Ack()) return 1;
	for(u8 i=0; i<LEN; i++)
	{
		IIC_Send_Byte(Wr_BUF[i]);
		if(IIC_Wait_Ack()) return 1;
	}
	IIC_Stop();
	return 0;
}

//return 0 means OK, 1 means Failed.
u8 I2C_ReadBytes(u8 ADDR, u8 REG, u8 LEN, u8* Rd_BUF)
{
	IIC_Start();
	IIC_Send_Byte((ADDR << 1) + 0);//0: Write Direction
	if(IIC_Wait_Ack()) return 1;
	IIC_Send_Byte(REG);
	if(IIC_Wait_Ack()) return 1;
	
	IIC_Start();//ReStart
	IIC_Send_Byte((ADDR << 1) + 1);//1: Read Direction
	if(IIC_Wait_Ack()) return 1;
	for(u8 i=0; i<LEN; i++)
	{
		if( i<(LEN-1) )
			Rd_BUF[i] = IIC_Read_Byte(1);//with ACK
		else
			Rd_BUF[i] = IIC_Read_Byte(0);//with nACK
	}
	IIC_Stop();
	return 0;
}



