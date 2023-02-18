#include "delay.h"
#include "si24r1.h"
#include "spi.h"

/*模块支持6发1收, 即1个发射地址, 6个接收地址(区分收到的数据来自哪一个发射机)
/ 发射机PTX为发射模式, 仅保留P0一个用于接收ACK的接收通道
/ 发射机的收发的地址应该一致
/ 接收机至多拥有6个接受地址, 且收到数据后按收到的地址当发送地址来发送ACK
/ 接收机原则上不用设置发送地址, 因为地址取决于收到的地址 */
// (0xE7是默认的地址)
const u8 TX_ADDRESS[5]={0xE7,0xE7,0xE7,0xE7,0xE7};
const u8 RX_ADDRESS[5]={0xE7,0xE7,0xE7,0xE7,0xE7};//{0xC2,0xC2,0xC2,0xC2,0xC2}


//PA.234分别是Flash,SDcard,oled的CS端, PC.4是NRF的CS端.
//暂定SI24R1_CE(PC6), SI24R1_CS(PC7), SI24R1_IRQ(PB12), 而spi2的另三根线会在SPI2_Init()中得到初始化
void si24r1_spi_init(void)//同时顺带着初始化CE端
{
	GPIO_InitTypeDef GPIO_InitStructure;
	//SPI_InitTypeDef SPI_InitStructure;
	
	RCC_APB2PeriphClockCmd(RF_CE_GPIO_CLK | RF_CS_GPIO_CLK | RF_IRQ_GPIO_CLK, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = RF_CE_GPIO_PIN;		//CE
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  	//推挽输出
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;		//影响功耗
 	GPIO_Init(RF_CE_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = RF_CS_GPIO_PIN;		//CS
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  	//推挽输出
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;		//影响功耗
 	GPIO_Init(RF_CS_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = RF_IRQ_GPIO_PIN;		//IRQ
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;		//上拉输入
	GPIO_Init(RF_IRQ_GPIO_PORT, &GPIO_InitStructure);

	
	GPIO_SetBits(RF_CS_GPIO_PORT, 	RF_CS_GPIO_PIN);		//拉高CS
	GPIO_ResetBits(RF_CE_GPIO_PORT, RF_CE_GPIO_PIN);		//拉低CE	
	GPIO_SetBits(RF_IRQ_GPIO_PORT, 	RF_IRQ_GPIO_PIN);		//拉高IRQ(低电平有效)

	SPI2_Init();//初始化SPI2 (函数内配置 Baud = 4.5M )
}

u8 si24r1_read_once(u8 reg)//单次读操作
{
	u8 value;
	SI24R1_CS = 0;
	SPI2_ReadWriteByte(reg);			//发送寄存器地址
	value = SPI2_ReadWriteByte(0xFF);	//发送空指令来读取内容
	SI24R1_CS = 1;
	return value;
}

void si24r1_write_once(u8 reg, u8 data)//单次写操作
{
	SI24R1_CS = 0;
	SPI2_ReadWriteByte(reg);			//发送寄存器地址
	SPI2_ReadWriteByte(data);			//发送想写入的值
	SI24R1_CS = 1;
}

void si24r1_read_buf(u8 reg, u8* pBUF, u8 len)//si24r1读取一串数据
{
	SI24R1_CS = 0;
	SPI2_ReadWriteByte(reg);
	for(u8 i=0; i<len; i++)
		pBUF[i] = SPI2_ReadWriteByte(0xFF);
	SI24R1_CS = 1;
}

void si24r1_write_buf(u8 reg, const u8* pBUF, u8 len)//si24r1写入一串数据
{
	SI24R1_CS = 0;
	SPI2_ReadWriteByte(reg);
	for(u8 i=0; i<len; i++)
		SPI2_ReadWriteByte(pBUF[i]);
	SI24R1_CS = 1;
}


/**
  * @brief  si24r1发送数据包,返回值为发送失败标志
  * @note	注意参数和函数内容受到宏定义控制
  * @param  *txBUF: 期望发送的数据
*			*rxBUF: 【可选项】接收数据
  * @retval RX_OK, TX_OK, DUPLEX_OK or FailedFlag.
  */
u8 si24r1_TX_packet(u8* txBUF
#if	DUPLEX_ACK_PL
	,u8* rxBUF	//可选参数
#endif
){
	u8 status;
	
	//SI24R1_CE = 0;
	si24r1_write_buf(WR_TX_PLOAD, txBUF, PAYLOAD_WIDTH);	//写TX有效数据32Byte(LSByte first)
	SI24R1_CE = 1;											//启动发送
	
	while(SI24R1_IRQ != 0);									//等待发送完成
	
	status = si24r1_read_once(STATUS);						//读状态寄存器STATUS
	si24r1_write_once(NRF_WRITE_REG + STATUS, status);		//将读到的status写入STATUS寄存器以清除状态
	SI24R1_CE = 0;										//回到StandBy状态
	
	if(status & MAX_TX)						//如果达到最大重发次数
	{
		si24r1_write_once(FLUSH_TX, 0xFF);			//清空TX_FIFO
		return MAX_TX;								//0x10代表MAX_TX
	}
#if DUPLEX_ACK_PL
	/* DUPLEX下PTX发送成功时RX_OK和TX_OK必然同时出现 */
	else if(status & RX_OK)					//如果收到的ACK中包含了回传的数据
	{
		u8 width = si24r1_read_once(R_RX_PL_WID);		//读取PRX回传数据的长度
		if(width > 32)
			si24r1_write_once(FLUSH_RX, 0xFF);		//长度错误, 删除收到的数据
		else
			si24r1_read_buf(RD_RX_PLOAD, rxBUF, width);
		return DUPLEX_OK;							//0x60代表同时出现RX_OK和TX_OK
	}
#endif
	else if(status & TX_OK)					//如果发送完成且收到了ACK
		return TX_OK;								//0x20代表TX_OK
	else
		return 0x0F;								//0x0F代表其他原因造成的发送失败
}


/**
  * @brief  si24r1接收数据包,返回值为接收失败标志
  * @note	注意参数和函数内容受到宏定义控制
  * @param  *rxBUF: 期望接收的数据
*			*txBUF: 【可选项】发送数据
  * @retval RX_OK, TX_OK, DUPLEX_OK or FailedFlag.
  */
u8 si24r1_RX_packet(u8* rxBUF
#if	DUPLEX_ACK_PL 
	,u8* txBUF	//可选参数
#endif
){
	u8 status;

	status = si24r1_read_once(STATUS);						//读状态寄存器REG_STATUS
	si24r1_write_once(NRF_WRITE_REG + STATUS, status);		//写状态寄存器,写1则清除对应的状态位
#if	DUPLEX_ACK_PL	
	if(status & RX_OK)						//如果收到PTX发送的数据
	{
		u8 width = si24r1_read_once(R_RX_PL_WID);		//读取PTX传送数据的长度
		
		/* 除了首次接收以外, DUPLEX下PRX接收成功时RX_OK和TX_OK必然同时出现 */
		si24r1_write_once(FLUSH_TX, 0xFF);
		si24r1_write_buf(W_ACK_PAYLOAD + 0, txBUF, ACK_PL_WIDTH);
		
		if(width > 32)
			si24r1_write_once(FLUSH_RX, 0xFF);		//长度错误, 删除收到的数据
		else
		{
			si24r1_read_buf(RD_RX_PLOAD, rxBUF, width);
			si24r1_write_once(FLUSH_RX, 0xFF);		//清空RX_FIFO
		}
		return DUPLEX_OK;							//0x60代表同时出现RX_OK和TX_OK
	}
	else
#endif
		 if(status & RX_OK)					//如果接收到数据(接收到首个数据时PRX不会触发TX_OK)
	{
		/* 从FIFO中读取已经接收到的数据 */
		si24r1_read_buf(RD_RX_PLOAD, rxBUF, PAYLOAD_WIDTH);
		
		/* 清空RX_FIFO */
		si24r1_write_once(FLUSH_RX, 0xFF);	//Commands里提到: 读取FIFO结束后FIFO的值会自动删除
		return RX_OK;								//0x40代表RX_OK
	}
	else
		return 0x0F;								//0x0F代表其他原因造成的接收失败
}

//接收机不需要设置TX_ADDR,它直接向收到的地址发送ACK
void si24r1_RX_mode()
{
	SI24R1_CE = 0;
	si24r1_write_buf(NRF_WRITE_REG + RX_ADDR_P0,RX_ADDRESS, 5);		//写入5Byte RX_P0地址
	si24r1_write_once(NRF_WRITE_REG + EN_AA, 	0x01);				//使能P0通道自动ACK
	si24r1_write_once(NRF_WRITE_REG + EN_RXADDR,0x01);				//使能P0的接收地址
	si24r1_write_once(NRF_WRITE_REG + RF_CH, 	RF_FREQUENCY);				//设置RF频段,默认为2402MHz
	si24r1_write_once(NRF_WRITE_REG + RF_SETUP, RF_bps_0Mbps | RF_PWR_7dBm);//设置 数据率,发射功率
	si24r1_write_once(NRF_WRITE_REG + RX_PW_P0, PAYLOAD_WIDTH);		//P0通道的数据宽度为20Byte
#if	DUPLEX_ACK_PL	
	si24r1_write_once(NRF_WRITE_REG + FEATURE, 0x06);	//使能动态数据长度DPL, 使能ACK with Payload
	si24r1_write_once(NRF_WRITE_REG + DYNPD, 0x01);		//使能DPL_P0
#endif	
	si24r1_write_once(NRF_WRITE_REG + STATUS, 	0xFF);//清除所用中断标志
	si24r1_write_once(NRF_WRITE_REG + CONFIG, 	0x0F);				//16位CRC,上电,开启中断,RX_mode
	SI24R1_CE = 1;													//进入接收模式
}

//发射机只有P0接收数据,所以要设置P0和TX_ADDR一样才能收到接收机发回的ACK
void si24r1_TX_mode()
{
	SI24R1_CE = 0;
	si24r1_write_buf(NRF_WRITE_REG + TX_ADDR, 	TX_ADDRESS, 5);		//写入5Byte TX地址,应等于接收机的RX地址
	si24r1_write_buf(NRF_WRITE_REG + RX_ADDR_P0,RX_ADDRESS, 5);		//写入5Byte的RX_P0地址(与TX一样)
	si24r1_write_once(NRF_WRITE_REG + EN_AA, 0x01);					//使能P0通道自动ACK
	si24r1_write_once(NRF_WRITE_REG + EN_RXADDR,0x01);				//使能P0的接收地址
	si24r1_write_once(NRF_WRITE_REG + SETUP_ART,AutoReTx_DELAY | AutoReTx_COUNT);	//自动重发间隔,最大重发次数
	si24r1_write_once(NRF_WRITE_REG + RF_CH, 	RF_FREQUENCY);						//设置RF频段,默认为2402MHz
	si24r1_write_once(NRF_WRITE_REG + RF_SETUP, RF_bps_0Mbps | RF_PWR_7dBm);		//设置1Mbps数据率,发射功率为0dBm
	si24r1_write_once(NRF_WRITE_REG + RX_PW_P0, PAYLOAD_WIDTH);		//P0通道的数据宽度为20Byte
#if	DUPLEX_ACK_PL	
	si24r1_write_once(NRF_WRITE_REG + FEATURE, 0x06);	//使能动态数据长度DPL, 使能ACK with Payload
	si24r1_write_once(NRF_WRITE_REG + DYNPD, 0x01);		//使能DPL_P0
#endif
	si24r1_write_once(NRF_WRITE_REG + STATUS, 	0xFF);//清除所用中断标志	
	si24r1_write_once(NRF_WRITE_REG + CONFIG, 	0x0E);				//16位CRC校验,上电,开启中断,TX_mode
	//SI24R1_CE = 1;												//进入发送模式
}




//检测NRF是否就位
u8 si24r1_detect()
{
	u8 buf[5]={0XA5,0XA5,0XA5,0XA5,0XA5};
	u8 i;   	 
	si24r1_write_buf(NRF_WRITE_REG + TX_ADDR, buf, 5);
	si24r1_read_buf(TX_ADDR, buf, 5);
	for(i=0;i<5;i++)
		if(buf[i] != 0XA5)
			break;	 							   
	if(i!=5)
		return 1;	
	return 0;
}

//接收模式下读取RSSI信号强度来倒推飞机是否失去了遥控器的控制(RX_mode)
//return 0 if RSSI < -64dBm
u8 si24r1_RX_RSSI()
{
	u8 index = si24r1_read_once(RX_RSSI);		//读取RSSI (RX_mode)
	if(index & 0x01)
		return 1;
	else
		return 0;
}




//NRF_IRQ中断脚配置
void NRF_IRQ_Config()
{
	EXTI_InitTypeDef EXTI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);//使能GPIOC的时钟
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//上拉
	GPIO_Init(GPIOC, &GPIO_InitStructure);			//PC4
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line4;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;	//下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}




u8 NRF_IRQ_TRIG = 0;


//NRF_IRQ中断(PC4)
void EXTI4_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line4)!=RESET)
	{	  
		NRF_IRQ_TRIG = 1;
	}
	EXTI_ClearITPendingBit(EXTI_Line4);
}

