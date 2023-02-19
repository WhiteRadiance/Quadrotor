#ifndef _SI24R1_H_
#define _SI24R1_H_
#include "sys.h"

#define	DUPLEX_ACK_PL	1	//1: ��һʱ��ʾPRX���ص�ACK����˳��PRX�˵���Ч����

//4��SPI�ӿ�
#define SI24R1_SCLK		PBout(13)	//SI24R1��spiʱ��
#define SI24R1_MOSI 	PBout(15)	//stm32�����SI24R1
#define SI24R1_MISO 	PBin(14)	//SI24R1�����stm32
/*#define SI24R1_CE		PAout(4)	//SI24R1��Enable,���ڿ����շ�ģʽ
#define SI24R1_CSN 		PCout(4) 	//spiƬѡ�ź�
#define SI24R1_IRQ  	PAin(1)		//�ж�����
*/
#define SI24R1_CE		PCout(6)	//SI24R1��Enable,���ڿ����շ�ģʽ
#define SI24R1_CS 		PCout(7) 	//spiƬѡ�ź�
#define SI24R1_IRQ  	PBin(12)	//�ж�����
			
			/* IRQ -->	/RX_DR: 	RX_DataReady,
						/TX_DS: 	TX_DataSent,
						/MAX_RT: 	MaxNum of reTX 	*/

// SI24R1_CE
#define RF_CE_GPIO_PORT		GPIOC
#define RF_CE_GPIO_CLK		RCC_APB2Periph_GPIOC	//��Ӧ��ʱ��
#define RF_CE_GPIO_PIN		GPIO_Pin_6				//PC6
// SI24R1_CS
#define RF_CS_GPIO_PORT		GPIOC
#define RF_CS_GPIO_CLK		RCC_APB2Periph_GPIOC	//��Ӧ��ʱ��
#define RF_CS_GPIO_PIN		GPIO_Pin_7				//PC7
// SI24R1_IRQ
#define RF_IRQ_GPIO_PORT	GPIOB
#define RF_IRQ_GPIO_CLK		RCC_APB2Periph_GPIOB	//��Ӧ��ʱ��
#define RF_IRQ_GPIO_PIN		GPIO_Pin_12				//PB12



#define ACK_PL_WIDTH	10		//PRX�ش������ݳ���	   10 Byte

#define PAYLOAD_WIDTH	20		//������Ч���ݳ���	   20 Byte
#define RT_ADDR_WIDTH	5		//�շ��ĵ�ַ���� 	 	5 Byte
#define AutoReTx_DELAY	0x50	//�Զ��ش���ʱ	  	 0x50=1500 us    0x30=1000 us    0x20=750us
								//ARD=500us is long enough for any ACK payload length in 1 or 2Mbps mode.
#define AutoReTx_COUNT	0x04	//����Զ��ش�����	    4 times
#define RF_FREQUENCY	2		//RFͨ��Ƶ��		 2402 MHz(Ĭ��Ϊ2402MHz)

#define SI_PWR_7dBm		0x07	//TX���书�� 7dBm for SI24R1
#define SI_PWR_3dBm		0x06	//TX���书�� 4dBm for SI24R1
#define SI_PWR_0dBm		0x03	//TX���书�� 0dBm for SI24R1
#define RF_PWR_0dBm		0x06	//TX���书�� 0dBm for NRF24L01 Plus

#define RF_bps_2Mbps	0x08	//RF�������� 2 Mbit/s
#define RF_bps_1Mbps	0x00	//RF�������� 1 Mbit/s
#define RF_bps_0Mbps	0x20	//RF�������� 250 Kbit/s

//STATUS
#define MAX_TX  		0x10	//�ﵽ����ʹ����ж�
#define TX_OK   		0x20	//TX��������ж�(���յ���ACK)
#define RX_OK   		0x40	//RX���յ������ж�
#define DUPLEX_OK   	0x60	//ACK��������ʱ��ͬʱ����TX_OK��RX_OK

//FIFO STATUS
#define TX_FULL  		0x20	//TX_FIFO ��
#define TX_EMPTY   		0x10	//TX_FIFO ��
#define RX_FULL   		0x02	//RX_FIFO ��
#define RX_EMPTY   		0x01	//RX_FIFO ��



/* -------  SPI��������  ------- */
// MSBit first, LSByte first
#define NRF_READ_REG 	0x00	//�����üĴ���,��5λΪ�Ĵ�����ַ
#define NRF_WRITE_REG   0x20  	//д���üĴ���,��5λΪ�Ĵ�����ַ
#define RD_RX_PLOAD     0x61  	//(RX mode)��RX��Ч����,1~32�ֽ�
#define WR_TX_PLOAD     0xA0  	//(TX mode)дTX��Ч����,1~32�ֽ�
#define FLUSH_TX        0xE1  	//(TX mode)���TX_FIFO�Ĵ���
#define FLUSH_RX        0xE2  	//(RX mode)���RX_FIFO�Ĵ���, ��Ҫ��ACK֮ǰʹ��
#define REUSE_TX_PL     0xE3  	//(PTX)����ʹ����һ������(���ڷ����),CEΪ��ʱ,���ݰ������Ϸ���
#define	R_RX_PL_WID		0x60	//��ȡRX_FIFO�е�һ����Ч���ݵ��ֽ���, 1�ֽ�
#define	W_ACK_PAYLOAD	0xA8	//(RX mode)10101xxx��ʾ��1~32�ֽ�xxxͨ�������ݺ�ACK�����һ��ش�
#define	W_TX_PL_NOACK	0xB0	//(TX mode)ָ���յ���ǰ���ݰ���RTX����ACK
#define NOP             0xFF  	//�ղ���,����������STATUS�Ĵ���(0x07)



/* -------  �Ĵ�����ַӳ��  ------- */
#define CONFIG          0x00	//���üĴ���			(PWR, RX/TX_mode, CRC, IRQ_Mask)
#define EN_AA           0x01	//ʹ��Auto_Ack	(P0-P5)
#define EN_RXADDR       0x02	//ʹ�ܽ��յ�ַ	(P0-P5)
#define SETUP_AW        0x03	//���õ�ַ���			(����ͨ��: 3/4/5 bytes)
#define SETUP_ART       0x04	//�����Զ��ط�			(�Զ��ط�������ARC; �Զ��ط���ʱARD: 250*x+86us)
#define RF_CH           0x05	//RFͨ��Channel			(����ͨ��Ƶ��: 2400MHz + (0~125)MHz)
#define RF_SETUP        0x06	//RF�������			(��������(1Mbit,2Mb,250kb),���书��)

#define 	STATUS      0x07	//״̬�Ĵ���(SPI���)	(RX_DR,  TX_DS,  MAX_RT,  RX_P_NO,  TX_FULL)

#define OBSERVE_TX      0x08	//�������ͳ�� 			(����������, �ط�������)
#define RX_RSSI         0x09	//�ź�ǿ��bit0(RX_mode)	(RSSI < -64dBm ʱΪ0, ����Ϊ1)
#define RX_ADDR_P0      0x0A	//P0���յ�ַ0xE7E7E7E7E7( MaxWidth is 5 Byte, ** LSByte first ** )
#define RX_ADDR_P1      0x0B	//P1���յ�ַ0xC2C2C2C2C2( LSByte first )
#define RX_ADDR_P2      0x0C	//P2���յ�ַ0xC3		(Only LSByte, �����ֽڵ���RX_ADDR_P1[39:8] )
#define RX_ADDR_P3      0x0D	//P3���յ�ַ0xC4		( [39:8] )
#define RX_ADDR_P4      0x0E	//P4���յ�ַ0xC5		( [39:8] )
#define RX_ADDR_P5      0x0F	//P5���յ�ַ0xC6		( [39:8] )
#define TX_ADDR         0x10	//���͵�ַ0xE7E7E7E7E7	(PTX Only, ShockBurstģʽ��,RX_ADDR_P0 = TX_ADDR)
#define RX_PW_P0        0x11	//P0_PayloadWidth	(1~32 Byte,����Ϊ0��Ƿ�)
#define RX_PW_P1        0x12	//P1_PayloadWidth	(1~32 B)
#define RX_PW_P2        0x13	//P2_PayloadWidth	(1~32 B)
#define RX_PW_P3        0x14	//P3_PayloadWidth	(1~32 B)
#define RX_PW_P4        0x15	//P4_PayloadWidth	(1~32 B)
#define RX_PW_P5        0x16	//P5_PayloadWidth	(1~32 B)
#define FIFO_STATUS		0x17	//FIFO״̬�Ĵ���	(TX_ReUSE,  TX_FULL,  TX_EMPTY,  RX_FULL,  RX_EMPTY)
#define DYNPD			0x1C	//ʹ�ܶ�̬���ݳ���	(p0-p5)
#define FEATURE			0x1D	//��̬ACK��ع���	(EN_DPL, EN_ACK_PAY, EN_DYN_ACK)


u8 si24r1_detect(void);	//return 1 if NRF is OK
u8 si24r1_RX_RSSI(void);//return 0 if RSSI < -64dBm

void si24r1_spi_init(void);
u8 si24r1_read_once(u8 reg);//���ζ�����
void si24r1_write_once(u8 reg, u8 data);//����д����
void si24r1_read_buf(u8 reg, u8* pBUF, u8 len);//si24r1��ȡһ������
void si24r1_write_buf(u8 reg, const u8* pBUF, u8 len);//si24r1д��һ������

void si24r1_RX_mode(void);
void si24r1_TX_mode(void);

//���ܿء�si24r1�������ݰ�,����ֵΪ����ʧ�ܱ�־
u8 si24r1_TX_packet(u8* txBUF
#if	DUPLEX_ACK_PL 
	,u8* rxBUF
#endif
);

//���ܿء�si24r1�������ݰ�,����ֵΪ����ʧ�ܱ�־
u8 si24r1_RX_packet(u8* rxBUF
#if	DUPLEX_ACK_PL 
	,u8* txBUF
#endif
);



void NRF_IRQ_Config(void);


#endif
