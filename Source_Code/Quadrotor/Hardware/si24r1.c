#include "delay.h"
#include "si24r1.h"
#include "spi.h"

/*ģ��֧��6��1��, ��1�������ַ, 6�����յ�ַ(�����յ�������������һ�������)
/ �����PTXΪ����ģʽ, ������P0һ�����ڽ���ACK�Ľ���ͨ��
/ ��������շ��ĵ�ַӦ��һ��
/ ���ջ�����ӵ��6�����ܵ�ַ, ���յ����ݺ��յ��ĵ�ַ�����͵�ַ������ACK
/ ���ջ�ԭ���ϲ������÷��͵�ַ, ��Ϊ��ַȡ�����յ��ĵ�ַ */
// (0xE7��Ĭ�ϵĵ�ַ)
const u8 TX_ADDRESS[5]={0xE7,0xE7,0xE7,0xE7,0xE7};
const u8 RX_ADDRESS[5]={0xE7,0xE7,0xE7,0xE7,0xE7};//{0xC2,0xC2,0xC2,0xC2,0xC2}


//PA.234�ֱ���Flash,SDcard,oled��CS��, PC.4��NRF��CS��.
//�ݶ�SI24R1_CE(PC6), SI24R1_CS(PC7), SI24R1_IRQ(PB12), ��spi2���������߻���SPI2_Init()�еõ���ʼ��
void si24r1_spi_init(void)//ͬʱ˳���ų�ʼ��CE��
{
	GPIO_InitTypeDef GPIO_InitStructure;
	//SPI_InitTypeDef SPI_InitStructure;
	
	RCC_APB2PeriphClockCmd(RF_CE_GPIO_CLK | RF_CS_GPIO_CLK | RF_IRQ_GPIO_CLK, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = RF_CE_GPIO_PIN;		//CE
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  	//�������
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;		//Ӱ�칦��
 	GPIO_Init(RF_CE_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = RF_CS_GPIO_PIN;		//CS
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  	//�������
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;		//Ӱ�칦��
 	GPIO_Init(RF_CS_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = RF_IRQ_GPIO_PIN;		//IRQ
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;		//��������
	GPIO_Init(RF_IRQ_GPIO_PORT, &GPIO_InitStructure);

	
	GPIO_SetBits(RF_CS_GPIO_PORT, 	RF_CS_GPIO_PIN);		//����CS
	GPIO_ResetBits(RF_CE_GPIO_PORT, RF_CE_GPIO_PIN);		//����CE	
	GPIO_SetBits(RF_IRQ_GPIO_PORT, 	RF_IRQ_GPIO_PIN);		//����IRQ(�͵�ƽ��Ч)

	SPI2_Init();//��ʼ��SPI2 (���������� Baud = 4.5M )
}

u8 si24r1_read_once(u8 reg)//���ζ�����
{
	u8 value;
	SI24R1_CS = 0;
	SPI2_ReadWriteByte(reg);			//���ͼĴ�����ַ
	value = SPI2_ReadWriteByte(0xFF);	//���Ϳ�ָ������ȡ����
	SI24R1_CS = 1;
	return value;
}

void si24r1_write_once(u8 reg, u8 data)//����д����
{
	SI24R1_CS = 0;
	SPI2_ReadWriteByte(reg);			//���ͼĴ�����ַ
	SPI2_ReadWriteByte(data);			//������д���ֵ
	SI24R1_CS = 1;
}

void si24r1_read_buf(u8 reg, u8* pBUF, u8 len)//si24r1��ȡһ������
{
	SI24R1_CS = 0;
	SPI2_ReadWriteByte(reg);
	for(u8 i=0; i<len; i++)
		pBUF[i] = SPI2_ReadWriteByte(0xFF);
	SI24R1_CS = 1;
}

void si24r1_write_buf(u8 reg, const u8* pBUF, u8 len)//si24r1д��һ������
{
	SI24R1_CS = 0;
	SPI2_ReadWriteByte(reg);
	for(u8 i=0; i<len; i++)
		SPI2_ReadWriteByte(pBUF[i]);
	SI24R1_CS = 1;
}


/**
  * @brief  si24r1�������ݰ�,����ֵΪ����ʧ�ܱ�־
  * @note	ע������ͺ��������ܵ��궨�����
  * @param  *txBUF: �������͵�����
*			*rxBUF: ����ѡ���������
  * @retval RX_OK, TX_OK, DUPLEX_OK or FailedFlag.
  */
u8 si24r1_TX_packet(u8* txBUF
#if	DUPLEX_ACK_PL
	,u8* rxBUF	//��ѡ����
#endif
){
	u8 status;
	
	//SI24R1_CE = 0;
	si24r1_write_buf(WR_TX_PLOAD, txBUF, PAYLOAD_WIDTH);	//дTX��Ч����32Byte(LSByte first)
	SI24R1_CE = 1;											//��������
	
	while(SI24R1_IRQ != 0);									//�ȴ��������
	
	status = si24r1_read_once(STATUS);						//��״̬�Ĵ���STATUS
	si24r1_write_once(NRF_WRITE_REG + STATUS, status);		//��������statusд��STATUS�Ĵ��������״̬
	SI24R1_CE = 0;										//�ص�StandBy״̬
	
	if(status & MAX_TX)						//����ﵽ����ط�����
	{
		si24r1_write_once(FLUSH_TX, 0xFF);			//���TX_FIFO
		return MAX_TX;								//0x10����MAX_TX
	}
#if DUPLEX_ACK_PL
	/* DUPLEX��PTX���ͳɹ�ʱRX_OK��TX_OK��Ȼͬʱ���� */
	else if(status & RX_OK)					//����յ���ACK�а����˻ش�������
	{
		u8 width = si24r1_read_once(R_RX_PL_WID);		//��ȡPRX�ش����ݵĳ���
		if(width > 32)
			si24r1_write_once(FLUSH_RX, 0xFF);		//���ȴ���, ɾ���յ�������
		else
			si24r1_read_buf(RD_RX_PLOAD, rxBUF, width);
		return DUPLEX_OK;							//0x60����ͬʱ����RX_OK��TX_OK
	}
#endif
	else if(status & TX_OK)					//�������������յ���ACK
		return TX_OK;								//0x20����TX_OK
	else
		return 0x0F;								//0x0F��������ԭ����ɵķ���ʧ��
}


/**
  * @brief  si24r1�������ݰ�,����ֵΪ����ʧ�ܱ�־
  * @note	ע������ͺ��������ܵ��궨�����
  * @param  *rxBUF: �������յ�����
*			*txBUF: ����ѡ���������
  * @retval RX_OK, TX_OK, DUPLEX_OK or FailedFlag.
  */
u8 si24r1_RX_packet(u8* rxBUF
#if	DUPLEX_ACK_PL 
	,u8* txBUF	//��ѡ����
#endif
){
	u8 status;

	status = si24r1_read_once(STATUS);						//��״̬�Ĵ���REG_STATUS
	si24r1_write_once(NRF_WRITE_REG + STATUS, status);		//д״̬�Ĵ���,д1�������Ӧ��״̬λ
#if	DUPLEX_ACK_PL	
	if(status & RX_OK)						//����յ�PTX���͵�����
	{
		u8 width = si24r1_read_once(R_RX_PL_WID);		//��ȡPTX�������ݵĳ���
		
		/* �����״ν�������, DUPLEX��PRX���ճɹ�ʱRX_OK��TX_OK��Ȼͬʱ���� */
		si24r1_write_once(FLUSH_TX, 0xFF);
		si24r1_write_buf(W_ACK_PAYLOAD + 0, txBUF, ACK_PL_WIDTH);
		
		if(width > 32)
			si24r1_write_once(FLUSH_RX, 0xFF);		//���ȴ���, ɾ���յ�������
		else
		{
			si24r1_read_buf(RD_RX_PLOAD, rxBUF, width);
			si24r1_write_once(FLUSH_RX, 0xFF);		//���RX_FIFO
		}
		return DUPLEX_OK;							//0x60����ͬʱ����RX_OK��TX_OK
	}
	else
#endif
		 if(status & RX_OK)					//������յ�����(���յ��׸�����ʱPRX���ᴥ��TX_OK)
	{
		/* ��FIFO�ж�ȡ�Ѿ����յ������� */
		si24r1_read_buf(RD_RX_PLOAD, rxBUF, PAYLOAD_WIDTH);
		
		/* ���RX_FIFO */
		si24r1_write_once(FLUSH_RX, 0xFF);	//Commands���ᵽ: ��ȡFIFO������FIFO��ֵ���Զ�ɾ��
		return RX_OK;								//0x40����RX_OK
	}
	else
		return 0x0F;								//0x0F��������ԭ����ɵĽ���ʧ��
}

//���ջ�����Ҫ����TX_ADDR,��ֱ�����յ��ĵ�ַ����ACK
void si24r1_RX_mode()
{
	SI24R1_CE = 0;
	si24r1_write_buf(NRF_WRITE_REG + RX_ADDR_P0,RX_ADDRESS, 5);		//д��5Byte RX_P0��ַ
	si24r1_write_once(NRF_WRITE_REG + EN_AA, 	0x01);				//ʹ��P0ͨ���Զ�ACK
	si24r1_write_once(NRF_WRITE_REG + EN_RXADDR,0x01);				//ʹ��P0�Ľ��յ�ַ
	si24r1_write_once(NRF_WRITE_REG + RF_CH, 	RF_FREQUENCY);				//����RFƵ��,Ĭ��Ϊ2402MHz
	si24r1_write_once(NRF_WRITE_REG + RF_SETUP, RF_bps_0Mbps | RF_PWR_7dBm);//���� ������,���书��
	si24r1_write_once(NRF_WRITE_REG + RX_PW_P0, PAYLOAD_WIDTH);		//P0ͨ�������ݿ��Ϊ20Byte
#if	DUPLEX_ACK_PL	
	si24r1_write_once(NRF_WRITE_REG + FEATURE, 0x06);	//ʹ�ܶ�̬���ݳ���DPL, ʹ��ACK with Payload
	si24r1_write_once(NRF_WRITE_REG + DYNPD, 0x01);		//ʹ��DPL_P0
#endif	
	si24r1_write_once(NRF_WRITE_REG + STATUS, 	0xFF);//��������жϱ�־
	si24r1_write_once(NRF_WRITE_REG + CONFIG, 	0x0F);				//16λCRC,�ϵ�,�����ж�,RX_mode
	SI24R1_CE = 1;													//�������ģʽ
}

//�����ֻ��P0��������,����Ҫ����P0��TX_ADDRһ�������յ����ջ����ص�ACK
void si24r1_TX_mode()
{
	SI24R1_CE = 0;
	si24r1_write_buf(NRF_WRITE_REG + TX_ADDR, 	TX_ADDRESS, 5);		//д��5Byte TX��ַ,Ӧ���ڽ��ջ���RX��ַ
	si24r1_write_buf(NRF_WRITE_REG + RX_ADDR_P0,RX_ADDRESS, 5);		//д��5Byte��RX_P0��ַ(��TXһ��)
	si24r1_write_once(NRF_WRITE_REG + EN_AA, 0x01);					//ʹ��P0ͨ���Զ�ACK
	si24r1_write_once(NRF_WRITE_REG + EN_RXADDR,0x01);				//ʹ��P0�Ľ��յ�ַ
	si24r1_write_once(NRF_WRITE_REG + SETUP_ART,AutoReTx_DELAY | AutoReTx_COUNT);	//�Զ��ط����,����ط�����
	si24r1_write_once(NRF_WRITE_REG + RF_CH, 	RF_FREQUENCY);						//����RFƵ��,Ĭ��Ϊ2402MHz
	si24r1_write_once(NRF_WRITE_REG + RF_SETUP, RF_bps_0Mbps | RF_PWR_7dBm);		//����1Mbps������,���书��Ϊ0dBm
	si24r1_write_once(NRF_WRITE_REG + RX_PW_P0, PAYLOAD_WIDTH);		//P0ͨ�������ݿ��Ϊ20Byte
#if	DUPLEX_ACK_PL	
	si24r1_write_once(NRF_WRITE_REG + FEATURE, 0x06);	//ʹ�ܶ�̬���ݳ���DPL, ʹ��ACK with Payload
	si24r1_write_once(NRF_WRITE_REG + DYNPD, 0x01);		//ʹ��DPL_P0
#endif
	si24r1_write_once(NRF_WRITE_REG + STATUS, 	0xFF);//��������жϱ�־	
	si24r1_write_once(NRF_WRITE_REG + CONFIG, 	0x0E);				//16λCRCУ��,�ϵ�,�����ж�,TX_mode
	//SI24R1_CE = 1;												//���뷢��ģʽ
}




//���NRF�Ƿ��λ
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

//����ģʽ�¶�ȡRSSI�ź�ǿ�������Ʒɻ��Ƿ�ʧȥ��ң�����Ŀ���(RX_mode)
//return 0 if RSSI < -64dBm
u8 si24r1_RX_RSSI()
{
	u8 index = si24r1_read_once(RX_RSSI);		//��ȡRSSI (RX_mode)
	if(index & 0x01)
		return 1;
	else
		return 0;
}




//NRF_IRQ�жϽ�����
void NRF_IRQ_Config()
{
	EXTI_InitTypeDef EXTI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);//ʹ��GPIOC��ʱ��
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//����
	GPIO_Init(GPIOC, &GPIO_InitStructure);			//PC4
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line4;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;	//�½��ش���
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}




u8 NRF_IRQ_TRIG = 0;


//NRF_IRQ�ж�(PC4)
void EXTI4_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line4)!=RESET)
	{	  
		NRF_IRQ_TRIG = 1;
	}
	EXTI_ClearITPendingBit(EXTI_Line4);
}

