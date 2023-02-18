#include "i2c.h"
#include "delay.h"


/**
  * @brief  对Bytes求对数(以2为底)
  * @note	必须保证整除, 如输入512, 则返回9
  * @param  Bytes: 字节数
  * @retval 以2为底的指数值
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
//将SDA配置为输入模式
static void SDA_IN(void)
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
static void SDA_OUT(void)
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
		I2C_GPIO_PORT->CRL |= (5<<(SDA_OFFSET)*4);			//通用开漏输出, 10MHz
		//I2C_GPIO_PORT->CRL |= (7<<(SDA_OFFSET-8)*4);		//通用开漏输出, 50MHz
		//I2C_GPIO_PORT->CRL |= (1<<(SDA_OFFSET)*4);		//通用推挽输出, 10MHz
		//I2C_GPIO_PORT->CRL |= (3<<(SDA_OFFSET)*4);		//通用推挽输出, 50MHz
	}
}
#endif

#define SDA_IN()  {GPIOC->CRL&=0XFFFFF0FF; GPIOC->CRL|=(u32)4<<8;}
#define SDA_OUT() {GPIOC->CRL&=0XFFFFF0FF; GPIOC->CRL|=(u32)5<<8;}


//初始化IIC
void IIC_Init(void)
{					     
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(I2C_GPIO_CLK, ENABLE);	
	   
	GPIO_InitStructure.GPIO_Pin = SCL_GPIO_PIN | SDA_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;			//开漏输出
	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;			//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(I2C_GPIO_PORT, &GPIO_InitStructure);

	//SCL_H_1;		//空闲时为高电平
	//SDA_H_1;		//空闲时为高电平
	IIC_SCL = 1;
	delay_us(1);
	IIC_SDA = 1;
	delay_us(1);
}


//产生IIC起始信号
void IIC_Start(void)
{
	SDA_OUT();     //sda线输出
	//SCL_H_1;
	//SDA_H_1;
	IIC_SDA = 1;
	IIC_SCL = 1;
	delay_us(3);
 	IIC_SDA = 0;//START: when CLK is high,DATA change from high to low 
	delay_us(4);
	IIC_SCL = 0;//钳住I2C总线，准备发送或接收数据
	delay_us(1);
}


//产生IIC停止信号
void IIC_Stop(void)
{
	SDA_OUT();//sda线输出
	IIC_SCL = 0;
	IIC_SDA = 0;//STOP: when CLK is high DATA change form low to high
 	delay_us(4);
	IIC_SCL = 1;
	delay_us(4);
	IIC_SDA = 1;//发送I2C总线结束信号
	delay_us(1);							   	
}


//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
u8 IIC_Wait_Ack(void)
{
	u8 ErrTime = 0;
	SDA_IN();      //SDA设置为输入  
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
	IIC_SCL=0;//时钟输出0 	   
	return 0;  
}


//产生ACK应答
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
	//IIC_SDA = 1;	//释放总线
}


//不产生ACK应答		    
void IIC_NAck(void)
{
	IIC_SCL = 0;
	SDA_OUT();
	IIC_SDA = 1;
	delay_us(2);
	IIC_SCL = 1;
	delay_us(3);
	IIC_SCL = 0;
	//不用释放总线了, 因为即将stop通信
}


//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答			  
void IIC_Send_Byte(u8 data)
{                        
    u8 t;   
	SDA_OUT(); 	    
    IIC_SCL=0;//拉低时钟开始数据传输
	delay_us(1);
    for(t=0;t<8;t++)
    {
		if(data & 0x80)
			IIC_SDA = 1;
		else
			IIC_SDA = 0;
		delay_us(2);
		IIC_SCL = 1;//让数据被从机取走
		delay_us(3);
		IIC_SCL = 0;
		delay_us(2);
		data <<= 1;
    }
	//IIC_SDA = 1;	//释放总线	
}


//读1个字节， /ack=1时，发送ACK，/ack=0，发送nACK   
u8 IIC_Read_Byte(u8 ack)
{
	u8 i, receive = 0;
	SDA_IN();		//SDA设置为输入
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
        IIC_Ack();	//发送ACK
    else
        IIC_NAck();	//发送nACK   
    return receive;
}

/*------------------------------------------------------------------------------------- /
/                          已经封装好的的读写操作相关函数                               /
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



