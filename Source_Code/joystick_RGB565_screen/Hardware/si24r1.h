#ifndef _SI24R1_H_
#define _SI24R1_H_
#include "sys.h"

#define	DUPLEX_ACK_PL	1	//1: 置一时表示PRX返回的ACK将会顺带PRX端的有效数据

//4线SPI接口
#define SI24R1_SCLK		PBout(13)	//SI24R1的spi时钟
#define SI24R1_MOSI 	PBout(15)	//stm32输出至SI24R1
#define SI24R1_MISO 	PBin(14)	//SI24R1输出至stm32
/*#define SI24R1_CE		PAout(4)	//SI24R1的Enable,用于控制收发模式
#define SI24R1_CSN 		PCout(4) 	//spi片选信号
#define SI24R1_IRQ  	PAin(1)		//中断请求
*/
#define SI24R1_CE		PCout(6)	//SI24R1的Enable,用于控制收发模式
#define SI24R1_CS 		PCout(7) 	//spi片选信号
#define SI24R1_IRQ  	PBin(12)	//中断请求
			
			/* IRQ -->	/RX_DR: 	RX_DataReady,
						/TX_DS: 	TX_DataSent,
						/MAX_RT: 	MaxNum of reTX 	*/

// SI24R1_CE
#define RF_CE_GPIO_PORT		GPIOC
#define RF_CE_GPIO_CLK		RCC_APB2Periph_GPIOC	//对应的时钟
#define RF_CE_GPIO_PIN		GPIO_Pin_6				//PC6
// SI24R1_CS
#define RF_CS_GPIO_PORT		GPIOC
#define RF_CS_GPIO_CLK		RCC_APB2Periph_GPIOC	//对应的时钟
#define RF_CS_GPIO_PIN		GPIO_Pin_7				//PC7
// SI24R1_IRQ
#define RF_IRQ_GPIO_PORT	GPIOB
#define RF_IRQ_GPIO_CLK		RCC_APB2Periph_GPIOB	//对应的时钟
#define RF_IRQ_GPIO_PIN		GPIO_Pin_12				//PB12



#define ACK_PL_WIDTH	10		//PRX回传的数据长度	   10 Byte

#define PAYLOAD_WIDTH	20		//单次有效数据长度	   20 Byte
#define RT_ADDR_WIDTH	5		//收发的地址长度 	 	5 Byte
#define AutoReTx_DELAY	0x50	//自动重传延时	  	 0x50=1500 us    0x30=1000 us    0x20=750us
								//ARD=500us is long enough for any ACK payload length in 1 or 2Mbps mode.
#define AutoReTx_COUNT	0x04	//最大自动重传次数	    4 times
#define RF_FREQUENCY	2		//RF通道频率		 2402 MHz(默认为2402MHz)

#define SI_PWR_7dBm		0x07	//TX发射功率 7dBm for SI24R1
#define SI_PWR_3dBm		0x06	//TX发射功率 4dBm for SI24R1
#define SI_PWR_0dBm		0x03	//TX发射功率 0dBm for SI24R1
#define RF_PWR_0dBm		0x06	//TX发射功率 0dBm for NRF24L01 Plus

#define RF_bps_2Mbps	0x08	//RF数据速率 2 Mbit/s
#define RF_bps_1Mbps	0x00	//RF数据速率 1 Mbit/s
#define RF_bps_0Mbps	0x20	//RF数据速率 250 Kbit/s

//STATUS
#define MAX_TX  		0x10	//达到最大发送次数中断
#define TX_OK   		0x20	//TX发送完成中断(且收到了ACK)
#define RX_OK   		0x40	//RX接收到数据中断
#define DUPLEX_OK   	0x60	//ACK包含数据时会同时出发TX_OK和RX_OK

//FIFO STATUS
#define TX_FULL  		0x20	//TX_FIFO 满
#define TX_EMPTY   		0x10	//TX_FIFO 空
#define RX_FULL   		0x02	//RX_FIFO 满
#define RX_EMPTY   		0x01	//RX_FIFO 空



/* -------  SPI控制命令  ------- */
// MSBit first, LSByte first
#define NRF_READ_REG 	0x00	//读配置寄存器,低5位为寄存器地址
#define NRF_WRITE_REG   0x20  	//写配置寄存器,低5位为寄存器地址
#define RD_RX_PLOAD     0x61  	//(RX mode)读RX有效数据,1~32字节
#define WR_TX_PLOAD     0xA0  	//(TX mode)写TX有效数据,1~32字节
#define FLUSH_TX        0xE1  	//(TX mode)清除TX_FIFO寄存器
#define FLUSH_RX        0xE2  	//(RX mode)清除RX_FIFO寄存器, 不要在ACK之前使用
#define REUSE_TX_PL     0xE3  	//(PTX)重新使用上一包数据(用于发射机),CE为高时,数据包被不断发送
#define	R_RX_PL_WID		0x60	//读取RX_FIFO中第一个有效数据的字节数, 1字节
#define	W_ACK_PAYLOAD	0xA8	//(RX mode)10101xxx表示将1~32字节xxx通道的数据和ACK打包并一起回传
#define	W_TX_PL_NOACK	0xB0	//(TX mode)指明收到当前数据包的RTX无需ACK
#define NOP             0xFF  	//空操作,可以用来读STATUS寄存器(0x07)



/* -------  寄存器地址映射  ------- */
#define CONFIG          0x00	//配置寄存器			(PWR, RX/TX_mode, CRC, IRQ_Mask)
#define EN_AA           0x01	//使能Auto_Ack	(P0-P5)
#define EN_RXADDR       0x02	//使能接收地址	(P0-P5)
#define SETUP_AW        0x03	//设置地址宽度			(所有通道: 3/4/5 bytes)
#define SETUP_ART       0x04	//建立自动重发			(自动重发计数器ARC; 自动重发延时ARD: 250*x+86us)
#define RF_CH           0x05	//RF通道Channel			(工作通道频率: 2400MHz + (0~125)MHz)
#define RF_SETUP        0x06	//RF相关设置			(传输速率(1Mbit,2Mb,250kb),发射功率)

#define 	STATUS      0x07	//状态寄存器(SPI输出)	(RX_DR,  TX_DS,  MAX_RT,  RX_P_NO,  TX_FULL)

#define OBSERVE_TX      0x08	//传输错误统计 			(丢包计数器, 重发计数器)
#define RX_RSSI         0x09	//信号强度bit0(RX_mode)	(RSSI < -64dBm 时为0, 否则为1)
#define RX_ADDR_P0      0x0A	//P0接收地址0xE7E7E7E7E7( MaxWidth is 5 Byte, ** LSByte first ** )
#define RX_ADDR_P1      0x0B	//P1接收地址0xC2C2C2C2C2( LSByte first )
#define RX_ADDR_P2      0x0C	//P2接收地址0xC3		(Only LSByte, 更高字节等于RX_ADDR_P1[39:8] )
#define RX_ADDR_P3      0x0D	//P3接收地址0xC4		( [39:8] )
#define RX_ADDR_P4      0x0E	//P4接收地址0xC5		( [39:8] )
#define RX_ADDR_P5      0x0F	//P5接收地址0xC6		( [39:8] )
#define TX_ADDR         0x10	//发送地址0xE7E7E7E7E7	(PTX Only, ShockBurst模式下,RX_ADDR_P0 = TX_ADDR)
#define RX_PW_P0        0x11	//P0_PayloadWidth	(1~32 Byte,设置为0则非法)
#define RX_PW_P1        0x12	//P1_PayloadWidth	(1~32 B)
#define RX_PW_P2        0x13	//P2_PayloadWidth	(1~32 B)
#define RX_PW_P3        0x14	//P3_PayloadWidth	(1~32 B)
#define RX_PW_P4        0x15	//P4_PayloadWidth	(1~32 B)
#define RX_PW_P5        0x16	//P5_PayloadWidth	(1~32 B)
#define FIFO_STATUS		0x17	//FIFO状态寄存器	(TX_ReUSE,  TX_FULL,  TX_EMPTY,  RX_FULL,  RX_EMPTY)
#define DYNPD			0x1C	//使能动态数据长度	(p0-p5)
#define FEATURE			0x1D	//动态ACK相关功能	(EN_DPL, EN_ACK_PAY, EN_DYN_ACK)


u8 si24r1_detect(void);	//return 1 if NRF is OK
u8 si24r1_RX_RSSI(void);//return 0 if RSSI < -64dBm

void si24r1_spi_init(void);
u8 si24r1_read_once(u8 reg);//单次读操作
void si24r1_write_once(u8 reg, u8 data);//单次写操作
void si24r1_read_buf(u8 reg, u8* pBUF, u8 len);//si24r1读取一串数据
void si24r1_write_buf(u8 reg, const u8* pBUF, u8 len);//si24r1写入一串数据

void si24r1_RX_mode(void);
void si24r1_TX_mode(void);

//【受控】si24r1发送数据包,返回值为发送失败标志
u8 si24r1_TX_packet(u8* txBUF
#if	DUPLEX_ACK_PL 
	,u8* rxBUF
#endif
);

//【受控】si24r1接收数据包,返回值为接收失败标志
u8 si24r1_RX_packet(u8* rxBUF
#if	DUPLEX_ACK_PL 
	,u8* txBUF
#endif
);



void NRF_IRQ_Config(void);


#endif
