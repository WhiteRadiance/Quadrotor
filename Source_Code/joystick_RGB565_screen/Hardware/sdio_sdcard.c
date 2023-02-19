#include "sdio_sdcard.h"
#include <stdlib.h>

//SDIO��ز�������, SDIO Static flags, TimeOut
//#define NULL 0
#define SDIO_STATIC_FLAGS               ((u32)0x000005FF)
#define SDIO_CMD0TIMEOUT                ((u32)0x00010000)
//#define SD_DATATIMEOUT                ((u32)0xFFFFFFFF)	-> inMasks for R6 Response
//#define SDIO_FIFO_ADDRESS             ((u32)0x40018080)	-> *.h file

//Mask for errors Card Status R1 (OCR Register)  
#define SD_OCR_ADDR_OUT_OF_RANGE        ((u32)0x80000000)
#define SD_OCR_ADDR_MISALIGNED          ((u32)0x40000000)
#define SD_OCR_BLOCK_LEN_ERR            ((u32)0x20000000)
#define SD_OCR_ERASE_SEQ_ERR            ((u32)0x10000000)
#define SD_OCR_BAD_ERASE_PARAM          ((u32)0x08000000)
#define SD_OCR_WRITE_PROT_VIOLATION     ((u32)0x04000000)
#define SD_OCR_LOCK_UNLOCK_FAILED       ((u32)0x01000000)
#define SD_OCR_COM_CRC_FAILED           ((u32)0x00800000)
#define SD_OCR_ILLEGAL_CMD              ((u32)0x00400000)
#define SD_OCR_CARD_ECC_FAILED          ((u32)0x00200000)
#define SD_OCR_CC_ERROR                 ((u32)0x00100000)
#define SD_OCR_GENERAL_UNKNOWN_ERROR    ((u32)0x00080000)
#define SD_OCR_STREAM_READ_UNDERRUN     ((u32)0x00040000)
#define SD_OCR_STREAM_WRITE_OVERRUN     ((u32)0x00020000)
#define SD_OCR_CID_CSD_OVERWRIETE       ((u32)0x00010000)
#define SD_OCR_WP_ERASE_SKIP            ((u32)0x00008000)
#define SD_OCR_CARD_ECC_DISABLED        ((u32)0x00004000)
#define SD_OCR_ERASE_RESET              ((u32)0x00002000)
#define SD_OCR_AKE_SEQ_ERROR            ((u32)0x00000008)
#define SD_OCR_ERRORBITS                ((u32)0xFDFFE008)

//Masks for R6 Response
#define SD_R6_GENERAL_UNKNOWN_ERROR     ((u32)0x00002000)
#define SD_R6_ILLEGAL_CMD               ((u32)0x00004000)
#define SD_R6_COM_CRC_FAILED            ((u32)0x00008000)


#define SD_VOLTAGE_WINDOW_SD            ((u32)0x80100000)
#define SD_HIGH_CAPACITY                ((u32)0x40000000)
#define SD_STD_CAPACITY                 ((u32)0x00000000)
#define SD_CHECK_PATTERN                ((u32)0x000001AA)		//1Ϊ��ѹ��Χ2.7-3.6V, AAΪ���ͨѶ
#define SD_VOLTAGE_WINDOW_MMC           ((u32)0x80FF8000)

#define SD_MAX_VOLT_TRIAL               ((u32)0x0000FFFF)
#define SD_ALLZERO                      ((u32)0x00000000)

#define SD_WIDE_BUS_SUPPORT             ((u32)0x00040000)
#define SD_SINGLE_BUS_SUPPORT           ((u32)0x00010000)
#define SD_CARD_LOCKED                  ((u32)0x02000000)
#define SD_CARD_PROGRAMMING             ((u32)0x00000007)
#define SD_CARD_RECEIVING               ((u32)0x00000006)
#define SD_DATATIMEOUT                  ((u32)0xFFFFFFFF)
#define SD_0TO7BITS                     ((u32)0x000000FF)
#define SD_8TO15BITS                    ((u32)0x0000FF00)
#define SD_16TO23BITS                   ((u32)0x00FF0000)
#define SD_24TO31BITS                   ((u32)0xFF000000)
#define SD_MAX_DATA_LENGTH              ((u32)0x01FFFFFF)

#define SD_HALFFIFO                     ((u32)0x00000008)
#define SD_HALFFIFOBYTES                ((u32)0x00000020)

//Command Class Supported  
#define SD_CCCC_LOCK_UNLOCK             ((u32)0x00000080)
#define SD_CCCC_WRITE_PROT              ((u32)0x00000040)
#define SD_CCCC_ERASE                   ((u32)0x00000020)

/** 
  * @brief  Following commands are SD Card Specific commands.
  *         SDIO_APP_CMD should be sent before sending these commands. 
  */
#define SDIO_SEND_IF_COND               ((uint32_t)0x00000008)



/* Private variables ---------------------------------------------------------*/
static u32 	CardType =  SDIO_STD_CAPACITY_SD_CARD_V1_x;	//SD������(Ĭ��Ϊ1.x��)

static u32 	/*CSD_Tab[4], CID_Tab[4], */RCA = 0;			//SD��CSD,CID�Լ���Ե�ַ(RCA)����
u32* CSD_Tab;
u32* CID_Tab;


//static u8 	SDSTATUS_Tab[16];		//�洢SD״̬, ��SSR��[512:400]
__IO u32 		StopCondition = 0;		//ֹͣ��������־, ����DMA����д
__IO SD_Error 	TransferError = SD_OK;	//���ڴ洢��������Ϣ
__IO u32 		TransferEnd = 0;		//���������־, ���жϷ������е���
SD_CardInfo 	SDCardInfo;				//SD���� CSD & CID ��Ϣ.(Card Specific Data)(Card ID number)

/*��ʼ��sdio�Ľṹ��*/
SDIO_InitTypeDef SDIO_InitStructure;
SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
SDIO_DataInitTypeDef SDIO_DataInitStructure;


/* Private function prototypes -----------------------------------------------*/
static SD_Error		CmdError(void);
static SD_Error 	CmdResp1Error(u8 cmd);
static SD_Error 	CmdResp7Error(void);
static SD_Error 	CmdResp3Error(void);
static SD_Error 	CmdResp2Error(void);
static SD_Error 	CmdResp6Error(u8 cmd, u16 *prca);
static SD_Error 	SDEnWideBus(FunctionalState NewState);
static SD_Error 	IsCardProgramming(u8 *pstatus);	//��鿨�Ƿ����ڱ��״̬(д����ʱ����Ҫ���ұ��)
static SD_Error 	FindSCR(u16 rca, u32 *pscr);
/* �·������ѻ��鵽*.h�ļ�, ��ȡ��static�ؼ��� */
//static u8 	convert_from_bytes_to_power_of_two(u16 NumberOfBytes);//�����(��2Ϊ��)

static void 	SDIO_SD_GPIO_Config(void);
static void 	SDIO_NVIC_Config(void);
static void 	SD_DMA_TxConfig(u32 *BufferSRC, u32 BufferSize);
static void 	SD_DMA_RxConfig(u32 *BufferDST, u32 BufferSize);



//SD_WriteBlock/SD_ReadBlock/Wr_Rd_MultiBlock()���õ�buf
//__align(4) u32 *tempbuff;

//SD_ReadDisk/SD_WriteDisk����ר��buf,�����������������ݻ�������ַ����4�ֽڶ����ʱ��,
//��Ҫ�õ�������,ȷ�����ݻ�������ַ��4�ֽڶ����.
//__align(4) u8 SDIO_DATA_BUFFER[512];



/** @defgroup STM32_EVAL_SDIO_SD_Private_Functions
  * @{
  */
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes the SD Card and put it into StandBy State (Ready for data transfer).
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_Init(void)
{
	CID_Tab = (u32*)malloc(sizeof(u32)*4);
	CSD_Tab = (u32*)malloc(sizeof(u32)*4);
	
	
	SD_Error errorstatus = SD_OK;
	
	//!< ����SDIO�ж�
	SDIO_NVIC_Config();
	
	//!< ���� DMA�ж�
	/*NVIC_InitStructure.NVIC_IRQChannel = SD_SDIO_DMA_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_Init(&NVIC_InitStructure);*/
	
	SDIO_SD_GPIO_Config();//SDIO�������ų�ʼ��
	
	SDIO_DeInit();//��λ����SDIO�ļĴ���
	
	/*�ϵ粢���п�ʶ������, ȷ�Ͽ��Ĳ�����ѹ*/
	errorstatus = SD_PowerON();
	/*!< CMD Response TimeOut (wait for CMDSENT flag) */
	if(errorstatus != SD_OK)	return(errorstatus);
	
	errorstatus = SD_InitializeCards(CID_Tab,CSD_Tab);//��ʼ��SD��
	/*!< CMD Response TimeOut (wait for CMDSENT flag) */
	if(errorstatus != SD_OK)	return(errorstatus);
	
	//�������ݴ���ģʽ, ��߶�д�ٶ�
	/*!< Configure the SDIO peripheral */
	/*!< SDIOCLK = HCLK, SDIO_CK = HCLK/(2 + SDIO_TRANSFER_CLK_DIV) */ 
	SDIO_InitStructure.SDIO_ClockDiv = SDIO_TRANSFER_CLK_DIV;
	SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
	SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;
	SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
	SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_1b;//��ʼ��ʱ��ʱΪ1bitģʽ
	SDIO_InitStructure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
	SDIO_Init(&SDIO_InitStructure);
	
	if(errorstatus == SD_OK)
	{
		/*----------------- Read CSD/CID MSD registers ------------------*/
		errorstatus = SD_GetCardInfo(&SDCardInfo, CID_Tab, CSD_Tab);
	}
	if(errorstatus == SD_OK)
	{
		/*------------------ use CMD7 to Select Card --------------------*/
		errorstatus = SD_SelectDeselect((u32)(SDCardInfo.RCA << 16));
	}
	if(errorstatus == SD_OK)
	{
		//���� 4bit ģʽ
		errorstatus = SD_EnableWideBusOperation(SDIO_BusWide_4b);
	}

	
	free(CID_Tab);
	free(CSD_Tab);
  return(errorstatus);
}

/**
  * @brief  ��ʼ��SDIO_SD�����GPIO����
  * @param  None
  * @retval None
  */
static void SDIO_SD_GPIO_Config(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	/*!< GPIOC and GPIOD Periph clock enable. */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | SD_DETECT_GPIO_CLK, ENABLE);
	/*!< Configure PC.08,09,10,11,12   pin: D0, D1, D2, D3, CLK pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	/*!< Configure PD.02 CMD line */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	/*!< Configure SD_DETECT_PIN pin: SD Card detect pin */
	//���Լ���һ���������ж�SD���Ƿ����(�����Ϊ�͵�ƽ)
	GPIO_InitStructure.GPIO_Pin = SD_DETECT_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(SD_DETECT_GPIO_PORT, &GPIO_InitStructure);// PA8, GPIO_Mode_IPU
	
	/*!< Enable the SDIO AHB Clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_SDIO, ENABLE);
	/*!< Enable the DMA2 Clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);//SDIO��DMAʹ�õ���DMA2��ͨ��4
}

static void SDIO_NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	
	NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
  * @brief  Gets the cuurent sd card data transfer status.
  * @param  None
  * @retval SDTransferState: Data Transfer state.
  *   This value can be: 
  *        - SD_TRANSFER_OK: 	No data transfer is acting
  *        - SD_TRANSFER_BUSY: 	Data transfer is acting
  */
SDTransferState SD_GetStatus(void)
{
	SDCardState cardstate =  SD_CARD_TRANSFER;

	cardstate = SD_GetState();
	
	if(cardstate == SD_CARD_TRANSFER)
		return(SD_TRANSFER_OK);
	else if(cardstate == SD_CARD_ERROR)
		return (SD_TRANSFER_ERROR);
	else
		return(SD_TRANSFER_BUSY);
}

/** //��ȡ CSR ��[12:9]Current_State����, ��״̬����״̬
  * @brief  Returns the current card's state.
  * @param  None
  * @retval SDCardState: SD Card Error or SD Card Current State.
  */
SDCardState SD_GetState(void)
{
  u32 resp1 = 0;
  
  //if(SD_Detect()== SD_PRESENT)
  //{
    if (SD_SendStatus(&resp1) != SD_OK)
    {
		return SD_CARD_ERROR;
    }
    else
    {
		return (SDCardState)((resp1 >> 9) & 0x0F);
    }
  //}
  /*else
  {
    return SD_CARD_ERROR;
  }*/
}

/** //��ȡ CSR ��[12:9]Current_State����
  * @brief  Returns the current card's status.
  * @param  pcardstatus: pointer to the buffer that will contain the SD card status (Card Status register).
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_SendStatus(u32 *pcardstatus)
{
	SD_Error errorstatus = SD_OK;

	SDIO->ARG = (u32)RCA << 16;
	SDIO->CMD = 0x44D;	//CMD13
  
	errorstatus = CmdResp1Error(SD_CMD_SEND_STATUS);
	if(errorstatus != SD_OK)	return(errorstatus);

	*pcardstatus = SDIO->RESP1;
	return(errorstatus);
}

/**
 * @brief  Detect if SD card is correctly plugged in the SD slot.
 * @param  None
 * @retval Return if SD is detected or not
 */
u8 SD_Detect(void)
{
	__IO u8 status = SD_PRESENT;

	/*!< Check GPIO to detect SD */
	if(GPIO_ReadInputDataBit(SD_DETECT_GPIO_PORT, SD_DETECT_PIN) != Bit_RESET)
	{
		status = SD_NOT_PRESENT;
	}
	return status;
}

/** //ȷ��SD���Ĺ�����ѹ������ʱ��
  * @brief  Enquires cards about their operating voltage and configures clock controls.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_PowerON(void)
{
	SD_Error errorstatus = SD_OK;
	u32 response = 0,count = 0, validvoltage = 0;
	u32 SDType = SD_STD_CAPACITY;
	
	/*!< Power ON Sequence ---------------------------------------------------------*/
	/*!< Configure the SDIO peripheral */
	/*!< SDIOCLK = HCLK, SDIO_CK = HCLK/(2 + SDIO_INIT_CLK_DIV) */
	/*!< SDIO_CK for initialization should not exceed 400 KHz */
	SDIO_InitStructure.SDIO_ClockDiv = SDIO_INIT_CLK_DIV;
	SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
	SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;
	SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
	SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_1b;
	SDIO_InitStructure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
	SDIO_Init(&SDIO_InitStructure);
	
	/*!< Set Power State to ON */
	SDIO_SetPowerState(SDIO_PowerState_ON);//ΪSDIO�ṩ��Դ
	/*!< Enable SDIO Clock */
	SDIO_ClockCmd(ENABLE);
	
/*!< CMD0: GO_IDLE_STATE --------------------------------------------------------------*/
	/*!< No CMD response required */
	//for(u8 i=0;i<74;i++)
	SDIO_CmdInitStructure.SDIO_Argument = 0x0;//�������������
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_GO_IDLE_STATE;//CMD0
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_No;//�������޶�Ӧ����Ӧ
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;//�رյȴ��ж�
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;//����ͨ��״̬��CPSM �ڷ�������֮ǰ�ȴ����ݴ������
	SDIO_SendCommand(&SDIO_CmdInitStructure);	
	errorstatus = CmdError();//�ȴ�CMD0�������
	/*!< CMD Response TimeOut (wait for CMDSENT flag) */
	if(errorstatus != SD_OK)	return(errorstatus);
	
/*!< CMD8: SEND_IF_COND ---------------------------------------------------------------*/
	/*!< Send CMD8 to verify SD card interface operating condition */
	/*!< Argument: - [31:12]: Reserved (shall be set to '0')
				- [11:8]: Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
				- [7:0]: Check Pattern (recommended 0xAA) */
	/*!< CMD Response: R7 */
	SDIO_CmdInitStructure.SDIO_Argument = SD_CHECK_PATTERN;//����0x000001AA,���SD���ӿ�����
	SDIO_CmdInitStructure.SDIO_CmdIndex = SDIO_SEND_IF_COND;//CMD8
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;//R7��48bit����Ӧ
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;			 					
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);		
	errorstatus = CmdResp7Error();//�ȴ�R7��Ӧ
	if(errorstatus==SD_OK)
	{
		CardType = SDIO_STD_CAPACITY_SD_CARD_V2_0;/*!< SD card 2.0 */
		SDType   = SD_HIGH_CAPACITY;//guess
	}
	else//is SD V1.x or MMC Card
	{
		//CardType = SDIO_MULTIMEDIA_CARD;//guess MMC Card
		/*!< CMD55 */
		SDIO_CmdInitStructure.SDIO_Argument = 0x00;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;//CMD55
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;//R1��48bit����Ӧ
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		errorstatus = CmdResp1Error(SD_CMD_APP_CMD);//�ȴ�R1��Ӧ
		/*if(errorstatus == SD_OK)
			CardType = SDIO_STD_CAPACITY_SD_CARD_V1_x;
		else
			CardType = SDIO_MULTIMEDIA_CARD;	//MMC������ӦCMD55
		return(SD_OK);
		*/
	}
	/*!< CMD55 */
	SDIO_CmdInitStructure.SDIO_Argument = 0x00;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;//CMD55
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;//R1��48bit����Ӧ
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);	
	errorstatus = CmdResp1Error(SD_CMD_APP_CMD);//�ȴ�R1��Ӧ
	
	/*!< If errorstatus is Command TimeOut, it is a MMC card */
	/*!< If errorstatus is SD_OK, it is a SD card: SD card 2.0 (voltage range mismatch) or SD card 1.x */
	if(errorstatus == SD_OK)
	{
		/*!< SD CARD */
		/*!< Send ACMD41 SD_APP_OP_COND with Argument 0x80100000 */
		while((!validvoltage) && (count < SD_MAX_VOLT_TRIAL))
		{
			/*!< SEND CMD55 APP_CMD with RCA as 0 */
			SDIO_CmdInitStructure.SDIO_Argument = 0x00;
			SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;//CMD55
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);

			errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
			if(errorstatus != SD_OK)	return(errorstatus);

			/*!< SEND ACMD41 with Argument 0x8010_0000 | 0x4000_0000 */
			SDIO_CmdInitStructure.SDIO_Argument = SD_VOLTAGE_WINDOW_SD | SDType;
			SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_APP_OP_COND;//ACMD41
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;//R3��48bit����Ӧ
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);

			errorstatus = CmdResp3Error();//�ȴ�R3��Ӧ
			if(errorstatus != SD_OK)	return(errorstatus);
			
			/* ����ѹƥ��, SD�����Զ���λpwr_up��־ */
			response = SDIO_GetResponse(SDIO_RESP1);//�õ�R3����ӦֵOCR
			/* ��ȡ����OCR�Ĵ�����pwr_upλ, �鿴�Ƿ��ѹƥ�� */
			validvoltage = (((response >> 31) == 1) ? 1 : 0);
			count++;
		}
		if (count >= SD_MAX_VOLT_TRIAL)
		{
			errorstatus = SD_INVALID_VOLTRANGE;
			return(errorstatus);
		}
		/* ��鿨������Ϣ�е�CCSλ */
		if (response &= SD_HIGH_CAPACITY)//0x4000_0000, [30:30]CCS = 1 is SDHC
		{
			CardType = SDIO_HIGH_CAPACITY_SD_CARD;//SDHC
		}
		else CardType = SDIO_STD_CAPACITY_SD_CARD_V2_0;//V2.0 SDSC
	}
	else
	{
		/*!< else MMC Card */
	}
	return(errorstatus);
}

/**
  * @brief  Turns the SDIO output signals off.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_PowerOFF(void)
{
	SD_Error errorstatus = SD_OK;
	/*!< Set Power State to OFF */
	SDIO_SetPowerState(SDIO_PowerState_OFF);
	return(errorstatus);
}

/** //��ʼ�����п�������״̬
  * @brief  Intializes all cards or single card as the case may be Card(s) come into standby state.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_InitializeCards(u32* CID_Tab, u32* CSD_Tab)
{
	SD_Error errorstatus = SD_OK;
	u16 rca = 0x01;
	
	if(SDIO_GetPowerState() == SDIO_PowerState_OFF)
	{
		errorstatus = SD_REQUEST_NOT_APPLICABLE;
		return(errorstatus);
	}

	if(SDIO_SECURE_DIGITAL_IO_CARD != CardType)
	{
		/*!< Send CMD2 ALL_SEND_CID */
		SDIO_CmdInitStructure.SDIO_Argument = 0x0;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_ALL_SEND_CID;//CMD2
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;//R2�ǳ���Ӧ
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);		
		errorstatus = CmdResp2Error();
		if(SD_OK != errorstatus)	return(errorstatus);
		
		/*!< Receive CID */
		CID_Tab[0] = SDIO_GetResponse(SDIO_RESP1);
		CID_Tab[1] = SDIO_GetResponse(SDIO_RESP2);
		CID_Tab[2] = SDIO_GetResponse(SDIO_RESP3);
		CID_Tab[3] = SDIO_GetResponse(SDIO_RESP4);
	}
	if((SDIO_STD_CAPACITY_SD_CARD_V1_x == CardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == CardType) 
		|| (SDIO_SECURE_DIGITAL_IO_COMBO_CARD == CardType) || (SDIO_HIGH_CAPACITY_SD_CARD == CardType))//�жϿ�������
	{
		/*!< Send CMD3 SET_RELATIVE_ADDR with argument 0 */
		/*!< SD Card publishes its RCA. */
		SDIO_CmdInitStructure.SDIO_Argument = 0x00;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;//CMD3
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;//R6�Ƕ���Ӧ
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		
		errorstatus = CmdResp6Error(SD_CMD_SET_REL_ADDR, &rca);//�õ� rca
		if(SD_OK != errorstatus)	return(errorstatus);
	}
	if(SDIO_SECURE_DIGITAL_IO_CARD != CardType)
	{
		RCA = rca;

		/*!< Send CMD9 SEND_CSD with argument as card's RCA */
		SDIO_CmdInitStructure.SDIO_Argument = (u32)(rca << 16);
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_CSD;//CMD9
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;//R2�ǳ���Ӧ
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);

		errorstatus = CmdResp2Error();
		if(SD_OK != errorstatus)	return(errorstatus);

		/*!< Receive CSD */
		CSD_Tab[0] = SDIO_GetResponse(SDIO_RESP1);
		CSD_Tab[1] = SDIO_GetResponse(SDIO_RESP2);
		CSD_Tab[2] = SDIO_GetResponse(SDIO_RESP3);
		CSD_Tab[3] = SDIO_GetResponse(SDIO_RESP4);
	}
	errorstatus = SD_OK; /*!< All cards get intialized */
	return(errorstatus);
}

/**
  * @brief  Returns information about specific card.
  * @param  cardinfo: pointer to a SD_CardInfo structure that contains all SD card information.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_GetCardInfo(SD_CardInfo *cardinfo, u32* CID_Tab, u32* CSD_Tab)
{
	u8 tmp = 0;
		
	cardinfo->CardType = (u8)CardType;
	cardinfo->RCA = (u16)RCA;
	/*!< SD_CSD */
	/*!< Byte 0 */
	tmp = (u8)((CSD_Tab[0] & 0xFF000000) >> 24);
	cardinfo->SD_csd.CSDStruct = (tmp & 0xC0) >> 6;
	cardinfo->SD_csd.SysSpecVersion = (tmp & 0x3C) >> 2;
	cardinfo->SD_csd.Reserved1 = tmp & 0x03;
	/*!< Byte 1 */
	tmp = (u8)((CSD_Tab[0] & 0x00FF0000) >> 16);
	cardinfo->SD_csd.TAAC = tmp;
	/*!< Byte 2 */
	tmp = (u8)((CSD_Tab[0] & 0x0000FF00) >> 8);
	cardinfo->SD_csd.NSAC = tmp;
	/*!< Byte 3 */
	tmp = (u8)(CSD_Tab[0] & 0x000000FF);
	cardinfo->SD_csd.MaxBusClkFrec = tmp;
	/*!< Byte 4 */
	tmp = (u8)((CSD_Tab[1] & 0xFF000000) >> 24);
	cardinfo->SD_csd.CardComdClasses = tmp << 4;
	/*!< Byte 5 */
	tmp = (u8)((CSD_Tab[1] & 0x00FF0000) >> 16);
	cardinfo->SD_csd.CardComdClasses |= (tmp & 0xF0) >> 4;
	cardinfo->SD_csd.RdBlockLen = tmp & 0x0F;
	/*!< Byte 6 */
	tmp = (u8)((CSD_Tab[1] & 0x0000FF00) >> 8);
	cardinfo->SD_csd.PartBlockRead = (tmp & 0x80) >> 7;
	cardinfo->SD_csd.WrBlockMisalign = (tmp & 0x40) >> 6;
	cardinfo->SD_csd.RdBlockMisalign = (tmp & 0x20) >> 5;
	cardinfo->SD_csd.DSRImpl = (tmp & 0x10) >> 4;
	cardinfo->SD_csd.Reserved2 = 0; /*!< Reserved */

	if((CardType==SDIO_STD_CAPACITY_SD_CARD_V1_x) || (CardType==SDIO_STD_CAPACITY_SD_CARD_V2_0))
	{
		cardinfo->SD_csd.DeviceSize = (tmp & 0x03) << 10;

		/*!< Byte 7 */
		tmp = (u8)(CSD_Tab[1] & 0x000000FF);
		cardinfo->SD_csd.DeviceSize |= (tmp) << 2;
		/*!< Byte 8 */
		tmp = (u8)((CSD_Tab[2] & 0xFF000000) >> 24);
		cardinfo->SD_csd.DeviceSize |= (tmp & 0xC0) >> 6;
		cardinfo->SD_csd.MaxRdCurrentVDDMin = (tmp & 0x38) >> 3;
		cardinfo->SD_csd.MaxRdCurrentVDDMax = (tmp & 0x07);
		/*!< Byte 9 */
		tmp = (u8)((CSD_Tab[2] & 0x00FF0000) >> 16);
		cardinfo->SD_csd.MaxWrCurrentVDDMin = (tmp & 0xE0) >> 5;
		cardinfo->SD_csd.MaxWrCurrentVDDMax = (tmp & 0x1C) >> 2;
		cardinfo->SD_csd.DeviceSizeMul = (tmp & 0x03) << 1;
		/*!< Byte 10 */
		tmp = (u8)((CSD_Tab[2] & 0x0000FF00) >> 8);
		cardinfo->SD_csd.DeviceSizeMul |= (tmp & 0x80) >> 7;		
		cardinfo->CardCapacity = (cardinfo->SD_csd.DeviceSize + 1) ;
		cardinfo->CardCapacity *= (1 << (cardinfo->SD_csd.DeviceSizeMul + 2));
		cardinfo->CardBlockSize = 1 << (cardinfo->SD_csd.RdBlockLen);
		cardinfo->CardCapacity *= cardinfo->CardBlockSize;
	}
	else if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)
	{
		/*!< Byte 7 */
		tmp = (u8)(CSD_Tab[1] & 0x000000FF);
		cardinfo->SD_csd.DeviceSize = (tmp & 0x3F) << 16;
		/*!< Byte 8 */
		tmp = (u8)((CSD_Tab[2] & 0xFF000000) >> 24);
		cardinfo->SD_csd.DeviceSize |= (tmp << 8);
		/*!< Byte 9 */
		tmp = (u8)((CSD_Tab[2] & 0x00FF0000) >> 16);
		cardinfo->SD_csd.DeviceSize |= (tmp);
		/*!< Byte 10 */
		tmp = (u8)((CSD_Tab[2] & 0x0000FF00) >> 8);		
		cardinfo->CardCapacity = (uint64_t)(cardinfo->SD_csd.DeviceSize + 1) * 512 * 1024;
		cardinfo->CardBlockSize = 512;
	}
	
	cardinfo->SD_csd.EraseGrSize = (tmp & 0x40) >> 6;
	cardinfo->SD_csd.EraseGrMul = (tmp & 0x3F) << 1;

	/*!< Byte 11 */
	tmp = (u8)(CSD_Tab[2] & 0x000000FF);
	cardinfo->SD_csd.EraseGrMul |= (tmp & 0x80) >> 7;
	cardinfo->SD_csd.WrProtectGrSize = (tmp & 0x7F);
	/*!< Byte 12 */
	tmp = (u8)((CSD_Tab[3] & 0xFF000000) >> 24);
	cardinfo->SD_csd.WrProtectGrEnable = (tmp & 0x80) >> 7;
	cardinfo->SD_csd.ManDeflECC = (tmp & 0x60) >> 5;
	cardinfo->SD_csd.WrSpeedFact = (tmp & 0x1C) >> 2;
	cardinfo->SD_csd.MaxWrBlockLen = (tmp & 0x03) << 2;
	/*!< Byte 13 */
	tmp = (u8)((CSD_Tab[3] & 0x00FF0000) >> 16);
	cardinfo->SD_csd.MaxWrBlockLen |= (tmp & 0xC0) >> 6;
	cardinfo->SD_csd.WriteBlockPaPartial = (tmp & 0x20) >> 5;
	cardinfo->SD_csd.Reserved3 = 0;
	cardinfo->SD_csd.ContentProtectAppli = (tmp & 0x01);
	/*!< Byte 14 */
	tmp = (u8)((CSD_Tab[3] & 0x0000FF00) >> 8);
	cardinfo->SD_csd.FileFormatGrouop = (tmp & 0x80) >> 7;
	cardinfo->SD_csd.CopyFlag = (tmp & 0x40) >> 6;
	cardinfo->SD_csd.PermWrProtect = (tmp & 0x20) >> 5;
	cardinfo->SD_csd.TempWrProtect = (tmp & 0x10) >> 4;
	cardinfo->SD_csd.FileFormat = (tmp & 0x0C) >> 2;
	cardinfo->SD_csd.ECC = (tmp & 0x03);
	/*!< Byte 15 */
	tmp = (u8)(CSD_Tab[3] & 0x000000FF);
	cardinfo->SD_csd.CSD_CRC = (tmp & 0xFE) >> 1;
	cardinfo->SD_csd.Reserved4 = 1;
	
	/*!< SD_CID */
	/*!< Byte 0 */
	tmp = (u8)((CID_Tab[0] & 0xFF000000) >> 24);
	cardinfo->SD_cid.ManufacturerID = tmp;
	/*!< Byte 1 */
	tmp = (u8)((CID_Tab[0] & 0x00FF0000) >> 16);
	cardinfo->SD_cid.OEM_AppliID = tmp << 8;
	/*!< Byte 2 */
	tmp = (u8)((CID_Tab[0] & 0x000000FF00) >> 8);
	cardinfo->SD_cid.OEM_AppliID |= tmp;
	/*!< Byte 3 */
	tmp = (u8)(CID_Tab[0] & 0x000000FF);
	cardinfo->SD_cid.ProdName1 = tmp << 24;
	/*!< Byte 4 */
	tmp = (u8)((CID_Tab[1] & 0xFF000000) >> 24);
	cardinfo->SD_cid.ProdName1 |= tmp << 16;
	/*!< Byte 5 */
	tmp = (u8)((CID_Tab[1] & 0x00FF0000) >> 16);
	cardinfo->SD_cid.ProdName1 |= tmp << 8;
	/*!< Byte 6 */
	tmp = (u8)((CID_Tab[1] & 0x0000FF00) >> 8);
	cardinfo->SD_cid.ProdName1 |= tmp;
	/*!< Byte 7 */
	tmp = (u8)(CID_Tab[1] & 0x000000FF);
	cardinfo->SD_cid.ProdName2 = tmp;
	/*!< Byte 8 */
	tmp = (u8)((CID_Tab[2] & 0xFF000000) >> 24);
	cardinfo->SD_cid.ProdRev = tmp;
	/*!< Byte 9 */
	tmp = (u8)((CID_Tab[2] & 0x00FF0000) >> 16);
	cardinfo->SD_cid.ProdSN = tmp << 24;
	/*!< Byte 10 */
	tmp = (u8)((CID_Tab[2] & 0x0000FF00) >> 8);
	cardinfo->SD_cid.ProdSN |= tmp << 16;
	/*!< Byte 11 */
	tmp = (u8)(CID_Tab[2] & 0x000000FF);
	cardinfo->SD_cid.ProdSN |= tmp << 8;
	/*!< Byte 12 */
	tmp = (u8)((CID_Tab[3] & 0xFF000000) >> 24);
	cardinfo->SD_cid.ProdSN |= tmp;
	/*!< Byte 13 */
	tmp = (u8)((CID_Tab[3] & 0x00FF0000) >> 16);
	cardinfo->SD_cid.Reserved1 |= (tmp & 0xF0) >> 4;
	cardinfo->SD_cid.ManufactDate = (tmp & 0x0F) << 8;
	/*!< Byte 14 */
	tmp = (u8)((CID_Tab[3] & 0x0000FF00) >> 8);
	cardinfo->SD_cid.ManufactDate |= tmp;
	/*!< Byte 15 */
	tmp = (u8)(CID_Tab[3] & 0x000000FF);
	cardinfo->SD_cid.CID_CRC = (tmp & 0xFE) >> 1;
	cardinfo->SD_cid.Reserved2 = 1;
	
	return(SD_OK);
}

/** //SSR
  * @brief  Returns the current SD status.
  * @param  WideMode: pointer to the buffer that contain the Sd Status (SSR).
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_GetCardStatus(SD_CardStatus *cardstatus)//--> u8 SDSTATUS_Tab[16]
{
	//to get [511:400] in 512bits (SSR)Sd Status Reg according to <SD Spec.P1>
	//ע����һ��״̬�Ĵ����� 32bits (CSR)Card Status Reg; CSRָ����״̬, SSRָ��SD�����������Ϣ
	
	SD_Error errorstatus = SD_OK;
	
	//�ڲ����� SD_SendSDStatus() ����
	
	return(errorstatus);
}

/** //SSR
  * @brief  Returns the current SD status.
  * @param  psdstatus: pointer to the buffer that will contain the SD card status 
  *         (SD Status register).
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_SendSDStatus(u32 *psdstatus)
{
	//��ȡ SSR ������
	//SD_GetCardStatus() �е����˱�����
	
	
	SD_Error errorstatus = SD_OK;
	u32 count = 0;

	if(SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
	{
		errorstatus = SD_LOCK_UNLOCK_FAILED;
		return(errorstatus);
	}

	/*!< Set block size for card if it is not equal to current block size for card. */
	SDIO_CmdInitStructure.SDIO_Argument = 64;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
	if(errorstatus != SD_OK)	return(errorstatus);

	/*!< CMD55 */
	SDIO_CmdInitStructure.SDIO_Argument = (u32) RCA << 16;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
	if(errorstatus != SD_OK)	return(errorstatus);
	
	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
	SDIO_DataInitStructure.SDIO_DataLength = 64;
	SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_64b;
	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
	SDIO_DataConfig(&SDIO_DataInitStructure);

	/*!< Send ACMD13 SD_APP_STAUS with argument as card's RCA.*/
	SDIO_CmdInitStructure.SDIO_Argument = 0;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_APP_STAUS;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_SD_APP_STAUS);
	if(errorstatus != SD_OK)	return(errorstatus);
	
	while(!(SDIO->STA &(SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND | SDIO_FLAG_STBITERR)))
	{
		if(SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)
		{
			for(count = 0; count < 8; count++)
			{
				*(psdstatus + count) = SDIO_ReadData();
			}
			psdstatus += 8;
		}
	}	
	if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
		errorstatus = SD_DATA_TIMEOUT;
		return(errorstatus);
	}
	else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
		errorstatus = SD_DATA_CRC_FAIL;
		return(errorstatus);
	}
	else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
		errorstatus = SD_RX_OVERRUN;
		return(errorstatus);
	}
	else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_STBITERR);
		errorstatus = SD_START_BIT_ERR;
		return(errorstatus);
	}
	while(SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)
	{
		*psdstatus = SDIO_ReadData();
		psdstatus++;
	}
	/*!< Clear all the static status flags*/
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);

	return(errorstatus);
}


/** //���ÿ������ݿ��(Ҫ��鿨�����Ƿ�֧��)
  * @brief  Enables wide bus opeartion for the requeseted card if supported by card.
  * @param  WideMode: Specifies the SD card wide bus mode. 
  *   This parameter can be one of the following values:
  *     @arg SDIO_BusWide_8b: 8-bit data transfer (Only for MMC)
  *     @arg SDIO_BusWide_4b: 4-bit data transfer
  *     @arg SDIO_BusWide_1b: 1-bit data transfer
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_EnableWideBusOperation(u32 WideMode)
{
	SD_Error errorstatus = SD_OK;
	/*!< MMC Card doesn't support this feature */
	if(SDIO_MULTIMEDIA_CARD == CardType)
	{
		errorstatus = SD_UNSUPPORTED_FEATURE;
		return(errorstatus);
	}
	else if((SDIO_STD_CAPACITY_SD_CARD_V1_x==CardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType) || (SDIO_HIGH_CAPACITY_SD_CARD==CardType))
	{
		if(SDIO_BusWide_8b == WideMode)
		{
			errorstatus = SD_UNSUPPORTED_FEATURE;
			return(errorstatus);
		}
		else if(SDIO_BusWide_4b == WideMode)
		{
			errorstatus = SDEnWideBus(ENABLE);
			if(SD_OK == errorstatus)
			{
				/*!< Configure the SDIO peripheral */
				SDIO_InitStructure.SDIO_ClockDiv = SDIO_TRANSFER_CLK_DIV; 
				SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
				SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;
				SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
				SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_4b;
				SDIO_InitStructure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
				SDIO_Init(&SDIO_InitStructure);
			}
		}
		else
		{
			errorstatus = SDEnWideBus(DISABLE);
			if (SD_OK == errorstatus)
			{
				/*!< Configure the SDIO peripheral */
				SDIO_InitStructure.SDIO_ClockDiv = SDIO_TRANSFER_CLK_DIV; 
				SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
				SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;
				SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
				SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_1b;
				SDIO_InitStructure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
				SDIO_Init(&SDIO_InitStructure);
			}
		}
	}
	return(errorstatus);
}

/** //����CMD7,ѡ����Ե�ַ(rca)Ϊaddr�Ŀ���ȡ��������; ���Ϊ0,�򶼲�ѡ��.
  * @brief  Selects or Deselects the corresponding card.
  * @param  addr: Address of the Card to be selected.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_SelectDeselect(u32 addr)
{
	SD_Error errorstatus = SD_OK;

	/*!< Send CMD7 SDIO_SEL_DESEL_CARD */
	SDIO_CmdInitStructure.SDIO_Argument =  addr;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEL_DESEL_CARD;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_SEL_DESEL_CARD);
	return(errorstatus);
}



/*-------------------  SD����д��������غ���  ---------------------  SD����д��������غ���  ---------------------*/


/**
  * @brief  ΪSDIO������������DMA2��CH4������, TX
  * @param  BufferSRC: pointer to the source buffer
  * @param  BufferSize: buffer size
  * @retval None
  */
static void SD_DMA_TxConfig(u32 *BufferSRC, u32 BufferSize)
{
	DMA_InitTypeDef DMA_InitStructure;

	DMA_ClearFlag(DMA2_FLAG_TC4 | DMA2_FLAG_TE4 | DMA2_FLAG_HT4 | DMA2_FLAG_GL4);
  
	/*!< DMA2 Channel4 disable */
	DMA_Cmd(DMA2_Channel4, DISABLE);

	/*!< DMA2 Channel4 Config */
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)SDIO_FIFO_ADDRESS;	//�����fifo��ַ
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)BufferSRC;				//��Ҫ���͵�Դ����
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;					//���ݴ��䷽�򣬴��ڴ浽����
	DMA_InitStructure.DMA_BufferSize = 	BufferSize/4;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	//�����ַ������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;				//�ڴ��ַ����
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;	//�������ݴ�СΪWord(32λ)
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;			//�ڴ����ݴ�СҲ��32λ
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;			//��ѭ��, ѭ��һ������ADC��
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;	//DMAͨ�����ȼ�Ϊ��
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;			//���� �ڴ浽�ڴ�ģʽ
	DMA_Init(DMA2_Channel4, &DMA_InitStructure);
	
	/*!< DMA2 Channel4 enable */
	DMA_Cmd(DMA2_Channel4, ENABLE);
}

/**
  * @brief  ΪSDIO������������DMA2��CH4������, RX
  * @param  BufferDST: pointer to the destination buffer
  * @param  BufferSize: buffer size
  * @retval None
  */
static void SD_DMA_RxConfig(u32 *BufferDST, u32 BufferSize)
{
	DMA_InitTypeDef DMA_InitStructure;

	DMA_ClearFlag(DMA2_FLAG_TC4 | DMA2_FLAG_TE4 | DMA2_FLAG_HT4 | DMA2_FLAG_GL4);
	/*!< DMA2 Channel4 disable */
	DMA_Cmd(DMA2_Channel4, DISABLE);

	/*!< DMA2 Channel4 Config */
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)SDIO_FIFO_ADDRESS;
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)BufferDST;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;					//���ݴ��䷽�򣬴����赽�ڴ�
	DMA_InitStructure.DMA_BufferSize = BufferSize/4;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	//�����ַ������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;				//�ڴ��ַ����
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA2_Channel4, &DMA_InitStructure);

	/*!< DMA2 Channel4 enable */
	DMA_Cmd(DMA2_Channel4, ENABLE); 
}

/**
  * @brief  Allows to read one block from a specified address in a card. The Data transfer is managed by DMA mode. 
  * @note   This operation should be followed by two functions to check if the DMA Controller and SD Card status.
     - SD_ReadWaitOperation():  this function insure that the DMA controller has finished all data transfer.
     - SD_GetStatus():          this function check that the SD Card has finished the data transfer and it is ready for data.            
  * @param  readbuff: pointer to the buffer that will contain the received data
  * @param  ReadAddr: Address from where data are to be read.  
  * @param  BlockSize: the SD card Data block size. The Block size should be 512.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_ReadBlock(u8 *readbuff, u32 ReadAddr, u16 BlockSize)
{
	SD_Error errorstatus = SD_OK;
	TransferError = SD_OK;
	TransferEnd = 0;		//���������־, ���жϷ���������1
	StopCondition = 0;
	
	SDIO->DCTRL = 0x0;		//��λ���ݿ��ƼĴ���

	if (CardType == SDIO_HIGH_CAPACITY_SD_CARD)
	{
		BlockSize = 512;
		ReadAddr /= 512;
	}
	
	//---------------add, ��û����һ�����׿�����DMA�����-----------------------
	/*!< Send CMD16 ���ÿ��С, SDHC�̶�Ϊ512�ֽڲ���Ӱ��*/
	SDIO_CmdInitStructure.SDIO_Argument = (u32)BlockSize;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;//CMD16
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);	
	errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
	if (errorstatus != SD_OK)	return(errorstatus);
	//--------------------------------------------------------------------------
	
	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
	SDIO_DataInitStructure.SDIO_DataLength = BlockSize;
	SDIO_DataInitStructure.SDIO_DataBlockSize = (u32) 9<<4;	// = SDIO_DataBlockSize_512b = 0x00000090
	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
	SDIO_DataConfig(&SDIO_DataInitStructure);
	
	/*!< Send CMD17 READ_SINGLE_BLOCK */
	SDIO_CmdInitStructure.SDIO_Argument = ReadAddr;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_READ_SINGLE_BLOCK;//CMD17
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_READ_SINGLE_BLOCK);
	if(errorstatus != SD_OK)	return(errorstatus);
	
	//SDIO_ITConfig(SDIO_IT_DATAEND, ENABLE);//�������ݴ�������ж�
	SDIO_ITConfig(SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_DATAEND | 
					SDIO_IT_RXOVERR | SDIO_IT_STBITERR, ENABLE);//����һ����Ҫ���ж�
    SDIO_DMACmd(ENABLE);//ʹ��SDIO��DMA����
    SD_DMA_RxConfig((u32 *)readbuff, BlockSize);//DMA2ͨ��4�Ľ�������
	
	return(errorstatus);
}

/**
  * @brief  Allows to read blocks from a specified address in a card. The Data transfer is managed by DMA mode. 
  * @note   This operation should be followed by two functions to check if the DMA Controller and SD Card status.
     - SD_ReadWaitOperation():  this function insure that the DMA controller has finished all data transfer.
     - SD_GetStatus():          this function check that the SD Card has finished the data transfer and it is ready for data.   
  * @param  readbuff: pointer to the buffer that will contain the received data.
  * @param  ReadAddr: Address from where data are to be read.
  * @param  BlockSize: the SD card Data block size. The Block size should be 512.
  * @param  NumberOfBlocks: number of blocks to be read.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_ReadMultiBlocks(u8 *readbuff, u32 ReadAddr, u16 BlockSize, u32 NumberOfBlocks)
{
	SD_Error errorstatus = SD_OK;
	TransferError = SD_OK;
	TransferEnd = 0;
	StopCondition = 1;
	SDIO->DCTRL = 0x0;		//��λ���ݿ��ƼĴ���
	
	if (CardType == SDIO_HIGH_CAPACITY_SD_CARD)
	{
		BlockSize = 512;
		ReadAddr /= 512;
	}
	
	/*!< use CMD16 to Set Block Size, SDHC�̶�Ϊ512�ֽڲ���Ӱ�� */
	SDIO_CmdInitStructure.SDIO_Argument = (u32) BlockSize;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;//CMD16
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
	if(SD_OK != errorstatus)	return(errorstatus);
	
	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
	SDIO_DataInitStructure.SDIO_DataLength = NumberOfBlocks * BlockSize;//���ڿ����ݴ���, ���ݳ��ȱ����ǿ鳤�ȵı���
	SDIO_DataInitStructure.SDIO_DataBlockSize = (u32) 9<<4;	// = SDIO_DataBlockSize_512b = 0x00000090
	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
	SDIO_DataConfig(&SDIO_DataInitStructure);

	/*!< Send CMD18 READ_MULT_BLOCK with argument data address */
	SDIO_CmdInitStructure.SDIO_Argument = (u32)ReadAddr;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_READ_MULT_BLOCK;//CMD18
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_READ_MULT_BLOCK);
	if(errorstatus != SD_OK)	return(errorstatus);

	//SDIO_ITConfig(SDIO_IT_DATAEND, ENABLE);//�������ݴ�������ж�
	SDIO_ITConfig(SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_DATAEND | 
					SDIO_IT_RXOVERR | SDIO_IT_STBITERR, ENABLE);//����һ����Ҫ���ж�
    SDIO_DMACmd(ENABLE);//ʹ��SDIO��DMA����
    SD_DMA_RxConfig((u32 *)readbuff, (NumberOfBlocks * BlockSize));//DMA2ͨ��4�Ľ���

	return(errorstatus);
}

/**
  * @brief  This function waits until the SDIO DMA data transfer is finished. 
  *         This function should be called after SDIO_ReadBlock() and SDIO_ReadMultiBlocks() function
			to insure that all data sent by the card are already transferred by the DMA controller.        
  * @param  None.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_WaitReadOperation(void)//Channel4 transfer complete flag.
{
	SD_Error errorstatus = SD_OK;
	while((DMA_GetFlagStatus(DMA2_FLAG_TC4)==RESET) && (TransferEnd==0) && (TransferError==SD_OK))
	{}
	if (TransferError != SD_OK)	return(TransferError);
		
	/*!< Clear all the static flags */
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);
	/*!< Wait till the card is in programming state */
	return(errorstatus);
}

/**
  * @brief  Allows to write one block starting from a specified address in a card. The Data transfer is managed by DMA mode.
  * @note   This operation should be followed by two functions to check the DMA Controller and SD Card status.
     - SD_ReadWaitOperation():  to insure that the DMA controller has finished all data transfer.
     - SD_GetStatus():          to check that the SD Card has finished the data transfer and is ready for data.      
  * @param  writebuff: pointer to the buffer that contain the data to be transferred.
  * @param  WriteAddr: Address from where data are to be read.   
  * @param  BlockSize: the SD card Data block size. The Block size should be 512.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_WriteBlock(u8 *writebuff, u32 WriteAddr, u16 BlockSize)
{
	SD_Error errorstatus = SD_OK;

	TransferError = SD_OK;
	TransferEnd = 0;		//���������־, ���жϷ���������1
	StopCondition = 0;
  
	SDIO->DCTRL = 0x0;		//��λ���ݿ��ƼĴ���

	if (CardType == SDIO_HIGH_CAPACITY_SD_CARD)
	{
		BlockSize = 512;
		WriteAddr /= 512;
	}
	
	//---------------add, ��û����һ�����׿�����DMA�����-----------------------
	/*!< Send CMD16 ���ÿ��С */
	SDIO_CmdInitStructure.SDIO_Argument = (u32)BlockSize;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;//CMD16
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);	
	errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
	if (errorstatus != SD_OK)	return(errorstatus);
	//--------------------------------------------------------------------------
	
	/*!< Send CMD24 WRITE_SINGLE_BLOCK */
	SDIO_CmdInitStructure.SDIO_Argument = WriteAddr;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_WRITE_SINGLE_BLOCK;//CMD24
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_WRITE_SINGLE_BLOCK);
	if(errorstatus != SD_OK)	return(errorstatus);

	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
	SDIO_DataInitStructure.SDIO_DataLength = BlockSize;
	SDIO_DataInitStructure.SDIO_DataBlockSize = (u32) 9<<4;	// = SDIO_DataBlockSize_512b = 0x00000090
	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
	SDIO_DataConfig(&SDIO_DataInitStructure);
	
	//SDIO_ITConfig(SDIO_IT_DATAEND, ENABLE);//�������ݴ�������ж�
	SDIO_ITConfig(SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_DATAEND | 
					SDIO_IT_TXUNDERR | SDIO_IT_STBITERR, ENABLE);//����һ����Ҫ���ж�
	SD_DMA_TxConfig((u32 *)writebuff, BlockSize);//DMA2ͨ��4�ķ�������
	SDIO_DMACmd(ENABLE);//ʹ��SDIO��DMA����
	
	return(errorstatus);
}

/**
  * @brief  Allows to write blocks starting from a specified address in a card. The Data transfer is managed by DMA mode only. 
  * @note   This operation should be followed by two functions to check if the DMA Controller and SD Card status.
     - SD_ReadWaitOperation():  this function insure that the DMA controller has finished all data transfer.
     - SD_GetStatus():          this function check that the SD Card has finished the data transfer and it is ready for data.     
  * @param  WriteAddr: Address from where data are to be read.
  * @param  writebuff: pointer to the buffer that contain the data to be transferred.
  * @param  BlockSize: the SD card Data block size. The Block size should be 512.
  * @param  NumberOfBlocks: number of blocks to be written.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_WriteMultiBlocks(u8 *writebuff, u32 WriteAddr, u16 BlockSize, u32 NumberOfBlocks)
{
	SD_Error errorstatus = SD_OK;
	TransferError = SD_OK;
	TransferEnd = 0;		//���������־, ���жϷ���������1
	StopCondition = 1;
	
	SDIO->DCTRL = 0x0;		//��λ���ݿ��ƼĴ���

	if (CardType == SDIO_HIGH_CAPACITY_SD_CARD)
	{
		BlockSize = 512;
		WriteAddr /= 512;
	}
	
	//---------------add, ��û����һ�����׿�����DMA�����--------------------------
	/*!< Send CMD16 ���ÿ��С, SDHC�̶�Ϊ512�ֽڲ���Ӱ��*/
	SDIO_CmdInitStructure.SDIO_Argument = (u32)BlockSize;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;//CMD16
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);	
	errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
	if (errorstatus != SD_OK)	return(errorstatus);
	//-----------------------------------------------------------------------------
	
	/*!< To improve performance */
	SDIO_CmdInitStructure.SDIO_Argument = (u32)(RCA << 16);
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;//CMD55
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
	if(errorstatus != SD_OK)	return(errorstatus);

	/*!< To improve performance */
	/* pre-erased���� ͨ��Ԥ��������߶��д����ٶ�*/
	SDIO_CmdInitStructure.SDIO_Argument = (u32)NumberOfBlocks;//��Ҫд��Ŀ����
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCK_COUNT;//ACMD23
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_SET_BLOCK_COUNT);
	if(errorstatus != SD_OK)	return(errorstatus);

	/*!< Send CMD25 WRITE_MULT_BLOCK with argument data address */
	SDIO_CmdInitStructure.SDIO_Argument = (u32)WriteAddr;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_WRITE_MULT_BLOCK;//CMD25
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_WRITE_MULT_BLOCK);
	if(SD_OK != errorstatus)	return(errorstatus);


	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
	SDIO_DataInitStructure.SDIO_DataLength = NumberOfBlocks * BlockSize;
	SDIO_DataInitStructure.SDIO_DataBlockSize = (u32) 9<<4;	// = SDIO_DataBlockSize_512b = 0x00000090
	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
	SDIO_DataConfig(&SDIO_DataInitStructure);
  
	//SDIO_ITConfig(SDIO_IT_DATAEND, ENABLE);//�������ݴ�������ж�
	SDIO_ITConfig(SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_DATAEND | 
					SDIO_IT_TXUNDERR | SDIO_IT_STBITERR, ENABLE);//����һ����Ҫ���ж�
	SDIO_DMACmd(ENABLE);//ʹ��SDIO��DMA����
	SD_DMA_TxConfig((u32 *)writebuff, (NumberOfBlocks * BlockSize));//DMA2ͨ��4�ķ�������

	return(errorstatus);
}

/**
  * @brief  This function waits until the SDIO DMA data transfer is finished. 
  *         This function should be called after SDIO_WriteBlock() and SDIO_WriteMultiBlocks() function
			to insure that all data sent by the card are already transferred by the DMA controller.        
  * @param  None.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_WaitWriteOperation(void)//Channel4 transfer complete flag.
{
	SD_Error errorstatus = SD_OK;
	while((DMA_GetFlagStatus(DMA2_FLAG_TC4)==RESET) && (TransferEnd==0) && (TransferError==SD_OK))
	{}
	if (TransferError != SD_OK)	return(TransferError);
		
	/*!< Clear all the static flags */
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);
	/*!< Wait till the card is in programming state */
	return(errorstatus);
}

/**
  * @brief  Allows to erase memory area specified for the given card.
  * @param  startaddr: the start address.
  * @param  endaddr: the end address.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_Erase(u32 startaddr, u32 endaddr)
{
	SD_Error errorstatus = SD_OK;
	u32 delay = 0;
	__IO u32 maxdelay = 0;
	u8 cardstate = 0;

	/*!< Check if the card coomnd class supports erase command */
	//if(((CSD_Tab[1] >> 20) & SD_CCCC_ERASE) == 0)
	if((SDCardInfo.SD_csd.CardComdClasses & SD_CCCC_ERASE) == 0)
	{
		errorstatus = SD_REQUEST_NOT_APPLICABLE;		//���˿��Ƿ�֧�ֲ�������
		return(errorstatus);
	}
	maxdelay = 120000 / ((SDIO->CLKCR & 0xFF) + 2);		//������ʱ
	if(SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
	{
		errorstatus = SD_LOCK_UNLOCK_FAILED;			//��鿨�Ƿ�����(����д)
		return(errorstatus);
	}
	if(CardType == SDIO_HIGH_CAPACITY_SD_CARD)			//SDHCֻ�ܰ�Block����, SDSCֻ�ܰ�Byte����
	{
		startaddr /= 512;
		endaddr /= 512;
	}
  
	/*!< According to SD spec 1.0 erase_group_START (CMD32) and erase_group_END(CMD33) */
	if((SDIO_STD_CAPACITY_SD_CARD_V1_x==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))
	{
		/*!< Send CMD32 SD_ERASE_GRP_START with argument as addr  */
		SDIO_CmdInitStructure.SDIO_Argument = startaddr;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_ERASE_GRP_START;//CMD32
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		errorstatus = CmdResp1Error(SD_CMD_SD_ERASE_GRP_START);
		if(errorstatus != SD_OK)	return(errorstatus);

		/*!< Send CMD33 SD_ERASE_GRP_END with argument as addr  */
		SDIO_CmdInitStructure.SDIO_Argument = endaddr;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_ERASE_GRP_END;//CMD33
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		errorstatus = CmdResp1Error(SD_CMD_SD_ERASE_GRP_END);
		if(errorstatus != SD_OK)	return(errorstatus);
	}
	/*!< Send CMD38 ERASE the selected range */
	SDIO_CmdInitStructure.SDIO_Argument = 0;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_ERASE;//CMD38
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_ERASE);
	if(errorstatus != SD_OK)	return(errorstatus);

	for (delay = 0; delay < maxdelay; delay++)		//delay
	{}

	/*!< Wait till the card is in programming state */
	errorstatus = IsCardProgramming(&cardstate);
	while((errorstatus==SD_OK) && ((SD_CARD_PROGRAMMING==cardstate)||(SD_CARD_RECEIVING==cardstate)))
	{
		errorstatus = IsCardProgramming(&cardstate);
	}
	return(errorstatus);
}

/**
  * @brief  Gets the cuurent data transfer state.
  * @param  None
  * @retval SDTransferState: Data Transfer state.
  *   This value can be: 
  *        - SD_TRANSFER_OK: No data transfer is acting
  *        - SD_TRANSFER_BUSY: Data transfer is acting
  */
SDTransferState SD_GetTransferState(void)
{
	if (SDIO->STA & (SDIO_FLAG_TXACT | SDIO_FLAG_RXACT))
	{
		return(SD_TRANSFER_BUSY);
	}
	else
	{
		return(SD_TRANSFER_OK);
	}
}

/**
  * @brief  Aborts an on-going data transfer.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_StopTransfer(void)
{
	SD_Error errorstatus = SD_OK;

	/*!< Send CMD12 STOP_TRANSMISSION */
	SDIO->ARG = 0x0;	//argument = 0
	SDIO->CMD = 0x44C;	//[11:8]=CPSM_enable, [7:6]=short_Resp, [5:0]=CMD_12
	
	errorstatus = CmdResp1Error(SD_CMD_STOP_TRANSMISSION);
	return(errorstatus);
}



/*------------------------------�жϺ���--------------------------------------SD��Ӧ�жϺ���---------------*/
/*----------------------------SD��Ӧ�жϺ���-------------------------------------�жϺ���------------------*/

/*
void SD_ProcessDMAIRQ(void)
{
	if(DMA2->LISR & SD_SDIO_DMA_FLAG_TCIF)
	{
		DMAEndOfTransfer = 0x01;
		DMA_ClearFlag(SD_SDIO_DMA_STREAM, SD_SDIO_DMA_FLAG_TCIF|SD_SDIO_DMA_FLAG_FEIF);
	}
}
//DMA�жϷ�����
void SD_SDIO_DMA_IRQHandler(void)
{
	//Process DMA2 Stream3 or DMA2 Stream6 Interrupt Sources
	SD_ProcessDMAIRQ();
}
*/

/** 
  * @brief  Allows to process all the interrupts that are high.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_ProcessIRQSrc(void)
{
	if(StopCondition == 1)		//���� ����д ʱ��==1
	{
		SDIO->ARG = 0x0;		//arg=0
		SDIO->CMD = 0x44C;		//[11:8]=CPSM_enable, [7:6]=short_Resp, [5:0]=CMD_12
		TransferError = CmdResp1Error(SD_CMD_STOP_TRANSMISSION);
	}
	else
	{
		TransferError = SD_OK;
	}
	SDIO_ClearITPendingBit(SDIO_IT_DATAEND);	//���ж�
	SDIO_ITConfig(SDIO_IT_DATAEND, DISABLE);	//�ر�SDIO�ж�ʹ��
	TransferEnd = 1;
	return(TransferError);
}

/**
  * @brief  Checks for error conditions for CMD0.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdError(void)
{
	SD_Error errorstatus = SD_OK;
	u32 timeout = SDIO_CMD0TIMEOUT;/*!< 10000 */
	while((timeout > 0) && (SDIO_GetFlagStatus(SDIO_FLAG_CMDSENT)==RESET))
	{
		timeout--;						//CMDSENT: �����ѷ���(������Ӧ)
	}
	if(timeout==0)
	{
		errorstatus = SD_CMD_RSP_TIMEOUT;
		return(errorstatus);
	}
	/*!< Clear all the static flags */
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��
	return(errorstatus);
}	 

/**
  * @brief  Checks for error conditions for R7 response.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp7Error(void)
{
	SD_Error errorstatus = SD_OK;
	u32 status;
	u32 timeout = SDIO_CMD0TIMEOUT;
	
	status = SDIO->STA;
 	while( !(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)) && (timeout > 0))
	//while( !(status & ((1<<0) | (1<<6) | (1<<2))) && (timeout > 0))
	{
		timeout--;
		status = SDIO->STA;
	}
	if((timeout==0)||(status & SDIO_FLAG_CTIMEOUT))		//if timeout
	{
		/*!< Card is not V2.0 complient or card does not support the voltage range */
		errorstatus = SD_CMD_RSP_TIMEOUT;
		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
		return(errorstatus);
	}
	/*else if(status & SDIO_FLAG_CCRCFAIL)				//if CRC-check failed
	{
		errorstatus = SD_CMD_CRC_FAIL;
		SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
		return(errorstatus);
	}*/	
	else if(status & SDIO_FLAG_CMDREND)//if received R7 succesfully
	{
		/*!< Card is SD V2.0 compliant */
		errorstatus= SD_OK;
		SDIO_ClearFlag(SDIO_FLAG_CMDREND);
		return(errorstatus);
 	}
	return(errorstatus);
}

/**
  * @brief  Checks for error conditions for R1 response.
  * @param  cmd: The sent command index.
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp1Error(u8 cmd)
{
	SD_Error errorstatus = SD_OK;
	u32 status;
	u32 response_r1;
	
	status = SDIO->STA;
	while(!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)))
	{
		status = SDIO->STA;
	} 
	if(status & SDIO_FLAG_CTIMEOUT)
	{
		errorstatus = SD_CMD_RSP_TIMEOUT;
 		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
		return(errorstatus);
	}
 	else if(status & SDIO_FLAG_CCRCFAIL)
	{
		errorstatus = SD_CMD_CRC_FAIL;
 		SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
		return(errorstatus);
	}
	
	/*!< Check response received is of desired command */
	if(SDIO_GetCommandResponse() != cmd)
	{
		errorstatus = SD_ILLEGAL_CMD;//���ƥ��
		return(errorstatus);
	}
  	/*!< Clear all the static flags */
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);

	/*!< We have received response, retrieve it for analysis. */
	response_r1 = SDIO_GetResponse(SDIO_RESP1);//ֻ��R2��Ϊ����Ӧ��ռ��128bit(RESP1~4),������Ӧֻռ��RESP1

	if((response_r1 & SD_OCR_ERRORBITS)==SD_ALLZERO)	return(SD_OK);
	
	if(response_r1 & SD_OCR_ADDR_OUT_OF_RANGE)				return(SD_ADDR_OUT_OF_RANGE);
	if(response_r1 & SD_OCR_ADDR_MISALIGNED)				return(SD_ADDR_MISALIGNED);
	if(response_r1 & SD_OCR_BLOCK_LEN_ERR)					return(SD_BLOCK_LEN_ERR);
	if(response_r1 & SD_OCR_ERASE_SEQ_ERR)					return(SD_ERASE_SEQ_ERR);
	if(response_r1 & SD_OCR_BAD_ERASE_PARAM)			return(SD_BAD_ERASE_PARAM);
	if(response_r1 & SD_OCR_WRITE_PROT_VIOLATION)		return(SD_WRITE_PROT_VIOLATION);
	if(response_r1 & SD_OCR_LOCK_UNLOCK_FAILED)			return(SD_LOCK_UNLOCK_FAILED);
	if(response_r1 & SD_OCR_COM_CRC_FAILED)				return(SD_COM_CRC_FAILED);
	if(response_r1 & SD_OCR_ILLEGAL_CMD)				return(SD_ILLEGAL_CMD);
	if(response_r1 & SD_OCR_CARD_ECC_FAILED)				return(SD_CARD_ECC_FAILED);
	if(response_r1 & SD_OCR_CC_ERROR)						return(SD_CC_ERROR);
	if(response_r1 & SD_OCR_GENERAL_UNKNOWN_ERROR)			return(SD_GENERAL_UNKNOWN_ERROR);
	if(response_r1 & SD_OCR_STREAM_READ_UNDERRUN)			return(SD_STREAM_READ_UNDERRUN);
	if(response_r1 & SD_OCR_STREAM_WRITE_OVERRUN)			return(SD_STREAM_WRITE_OVERRUN);
	if(response_r1 & SD_OCR_CID_CSD_OVERWRIETE)			return(SD_CID_CSD_OVERWRITE);
	if(response_r1 & SD_OCR_WP_ERASE_SKIP)				return(SD_WP_ERASE_SKIP);
	if(response_r1 & SD_OCR_CARD_ECC_DISABLED)			return(SD_CARD_ECC_DISABLED);
	if(response_r1 & SD_OCR_ERASE_RESET)				return(SD_ERASE_RESET);
	if(response_r1 & SD_OCR_AKE_SEQ_ERROR)				return(SD_AKE_SEQ_ERROR);

	return(errorstatus);
}

/**
  * @brief  Checks for error conditions for R3 (OCR) response.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp3Error(void)
{
	SD_Error errorstatus = SD_OK;
	u32 status;
	
	status = SDIO->STA;
	while(!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)))
	{
		status = SDIO->STA;
	}
	if(status & SDIO_FLAG_CTIMEOUT)
	{
		errorstatus = SD_CMD_RSP_TIMEOUT;
		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
		return(errorstatus);
	}
	/*!< Clear all the static flags */
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);
	return(errorstatus);
}

/**
  * @brief  Checks for error conditions for R2 (CID or CSD) response.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp2Error(void)
{
	SD_Error errorstatus = SD_OK;
	u32 status;
	
	status = SDIO->STA;
	while(!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)))
	{
		status = SDIO->STA;
	}
	if(status & SDIO_FLAG_CTIMEOUT)
	{
		errorstatus = SD_CMD_RSP_TIMEOUT;
		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
		return(errorstatus);
	}
	else if(status & SDIO_FLAG_CCRCFAIL)
	{
		errorstatus = SD_CMD_CRC_FAIL;
		SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
		return(errorstatus);
	}
	/*!< Clear all the static flags */
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);
	return(errorstatus);
}

/**
  * @brief  Checks for error conditions for R6 (RCA) response.
  * @param  cmd: The sent command index.
  * @param  prca: pointer to the variable that will contain the SD card relative address RCA. 
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp6Error(u8 cmd, u16 *prca)//prca: pointer of RCA
{
	SD_Error errorstatus = SD_OK;
	u32 status;
	u32 response_r1;

	status = SDIO->STA;
	while(!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CTIMEOUT | SDIO_FLAG_CMDREND)))
	{
		status = SDIO->STA;
	}
	if(status & SDIO_FLAG_CTIMEOUT)
	{
		errorstatus = SD_CMD_RSP_TIMEOUT;
		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
		return(errorstatus);
	}
	else if(status & SDIO_FLAG_CCRCFAIL)
	{
		errorstatus = SD_CMD_CRC_FAIL;
		SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
		return(errorstatus);
	}

	/*!< Check response received is of desired command */
	if(SDIO_GetCommandResponse() != cmd)
	{
		errorstatus = SD_ILLEGAL_CMD;
		return(errorstatus);
	}
	/*!< Clear all the static flags */
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);
	
	/*!< We have received response, retrieve it. */
	response_r1 = SDIO_GetResponse(SDIO_RESP1);

	if(SD_ALLZERO==(response_r1 & (SD_R6_GENERAL_UNKNOWN_ERROR | SD_R6_ILLEGAL_CMD | SD_R6_COM_CRC_FAILED)))
	{
		*prca = (u16)(response_r1 >> 16);//RESP1[31:16]is RCA,[15:0]is card status.
		return(errorstatus);
	}
	if(response_r1 & SD_R6_GENERAL_UNKNOWN_ERROR)	return(SD_GENERAL_UNKNOWN_ERROR);
	if(response_r1 & SD_R6_ILLEGAL_CMD)				return(SD_ILLEGAL_CMD);
	if(response_r1 & SD_R6_COM_CRC_FAILED)			return(SD_COM_CRC_FAILED);

	return(errorstatus);
}

/**
  * @brief  Enables or disables the SDIO wide bus mode.
  * @param  NewState: new state of the SDIO wide bus mode.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error SDEnWideBus(FunctionalState NewState)
{
	SD_Error errorstatus = SD_OK;
	u32 scr[2] = {0, 0};

	if (SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
	{
		errorstatus = SD_LOCK_UNLOCK_FAILED;
		return(errorstatus);
	}
	/*!< Get SCR Register */
	errorstatus = FindSCR(RCA, scr);
	if (errorstatus != SD_OK)
	{
		return(errorstatus);
	}
	/*!< If wide bus operation to be enabled */
	if(NewState == ENABLE)
	{
		/*!< If requested card supports wide bus operation */
		if((scr[1] & SD_WIDE_BUS_SUPPORT) != SD_ALLZERO)
		{
			/*!< Send CMD55 APP_CMD with argument as card's RCA.*/
			SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) RCA << 16;
			SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);
			errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
			if(errorstatus != SD_OK)	return(errorstatus);

			/*!< Send ACMD6 APP_CMD with argument as 2 for wide bus mode */
			SDIO_CmdInitStructure.SDIO_Argument = 0x2;
			SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_SD_SET_BUSWIDTH;//ACMD6
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);
			errorstatus = CmdResp1Error(SD_CMD_APP_SD_SET_BUSWIDTH);
			if(errorstatus != SD_OK)
			{
				return(errorstatus);
			}
			return(errorstatus);
		}
		else
		{
			errorstatus = SD_REQUEST_NOT_APPLICABLE;
			return(errorstatus);
		}
	}
	else /*!< If wide bus operation to be disabled */
	{		
		/*!< Send CMD55 APP_CMD with argument as card's RCA.*/
		SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) RCA << 16;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
		if(errorstatus != SD_OK)	return(errorstatus);

		/*!< Send ACMD6 APP_CMD with argument as 0 for 1bit bus mode */
		SDIO_CmdInitStructure.SDIO_Argument = 0x00;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_SD_SET_BUSWIDTH;//ACMD6
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		errorstatus = CmdResp1Error(SD_CMD_APP_SD_SET_BUSWIDTH);
		if(errorstatus != SD_OK)
		{
			return(errorstatus);
		}
		return(errorstatus);		
	}
}

/**
  * @brief  Checks if the SD card is in programming state.
  * @param  pstatus: pointer to the variable that will contain the SD card state.
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error IsCardProgramming(u8 *pstatus)
{
	SD_Error errorstatus = SD_OK;
	__IO u32 respR1 = 0, status = 0;

	SDIO_CmdInitStructure.SDIO_Argument = (u32) RCA << 16;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);

	status = SDIO->STA;
	while(!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)))
	{
		status = SDIO->STA;
	}
	if(status & SDIO_FLAG_CTIMEOUT)
	{
		errorstatus = SD_CMD_RSP_TIMEOUT;
		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
		return(errorstatus);
	}
	else if(status & SDIO_FLAG_CCRCFAIL)
	{
		errorstatus = SD_CMD_CRC_FAIL;
		SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
		return(errorstatus);
	}

	status = (u32)SDIO_GetCommandResponse();
	/*!< Check response received is of desired command */
	if(status != SD_CMD_SEND_STATUS)
	{
		errorstatus = SD_ILLEGAL_CMD;
		return(errorstatus);
	}

	/*!< Clear all the static flags */
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);

	/*!< We have received response, retrieve it for analysis  */
	respR1 = SDIO_GetResponse(SDIO_RESP1);
	/*!< Find out card status */
	*pstatus = (u8) ((respR1 >> 9) & 0x0000000F);

	if((respR1 & SD_OCR_ERRORBITS) == SD_ALLZERO)	return(errorstatus);
	if(respR1 & SD_OCR_ADDR_OUT_OF_RANGE)			return(SD_ADDR_OUT_OF_RANGE);
	if(respR1 & SD_OCR_ADDR_MISALIGNED)				return(SD_ADDR_MISALIGNED);
	if(respR1 & SD_OCR_BLOCK_LEN_ERR)				return(SD_BLOCK_LEN_ERR);
	if(respR1 & SD_OCR_ERASE_SEQ_ERR)				return(SD_ERASE_SEQ_ERR);
	if(respR1 & SD_OCR_BAD_ERASE_PARAM)			return(SD_BAD_ERASE_PARAM);
	if(respR1 & SD_OCR_WRITE_PROT_VIOLATION)	return(SD_WRITE_PROT_VIOLATION);
	if(respR1 & SD_OCR_LOCK_UNLOCK_FAILED)		return(SD_LOCK_UNLOCK_FAILED);
	if(respR1 & SD_OCR_COM_CRC_FAILED)			return(SD_COM_CRC_FAILED);
	if(respR1 & SD_OCR_ILLEGAL_CMD)				return(SD_ILLEGAL_CMD);
	if(respR1 & SD_OCR_CARD_ECC_FAILED)				return(SD_CARD_ECC_FAILED);
	if(respR1 & SD_OCR_CC_ERROR)					return(SD_CC_ERROR);
	if(respR1 & SD_OCR_GENERAL_UNKNOWN_ERROR)		return(SD_GENERAL_UNKNOWN_ERROR);
	if(respR1 & SD_OCR_STREAM_READ_UNDERRUN)		return(SD_STREAM_READ_UNDERRUN);
	if(respR1 & SD_OCR_STREAM_WRITE_OVERRUN)		return(SD_STREAM_WRITE_OVERRUN);  
	if(respR1 & SD_OCR_CID_CSD_OVERWRIETE)		return(SD_CID_CSD_OVERWRITE);
	if(respR1 & SD_OCR_WP_ERASE_SKIP)			return(SD_WP_ERASE_SKIP);
	if(respR1 & SD_OCR_CARD_ECC_DISABLED)		return(SD_CARD_ECC_DISABLED);
	if(respR1 & SD_OCR_ERASE_RESET)				return(SD_ERASE_RESET);
	if(respR1 & SD_OCR_AKE_SEQ_ERROR)			return(SD_AKE_SEQ_ERROR);
	
	return(errorstatus);
}

/** //��ȡSdcard Config Register������
  * @brief  Find the SD card SCR register value.
  * @param  rca: selected card address.
  * @param  pscr: pointer to the buffer that will contain the SCR value.
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error FindSCR(u16 rca, u32 *pscr)
{
	u32 index = 0;
	SD_Error errorstatus = SD_OK;
	u32 tempscr[2] = {0, 0};

	/*!< Set Block Size To 8 Bytes */
	/*!< Send CMD16 SD_CMD_SET_BLOCKLEN to set block size as 8 Byte */
	SDIO_CmdInitStructure.SDIO_Argument = (u32)8;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;//CMD16
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
	if(errorstatus != SD_OK)	return(errorstatus);


	/*!< Send CMD55 APP_CMD with argument as card's RCA */
	SDIO_CmdInitStructure.SDIO_Argument = (u32)RCA << 16;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;//CMD55
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
	if(errorstatus != SD_OK)	return(errorstatus);

	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
	SDIO_DataInitStructure.SDIO_DataLength = 8;//8���ֽڳ���,blockΪ8�ֽ�
	SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_8b;
	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;//SD����SDIO.
	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
	SDIO_DataConfig(&SDIO_DataInitStructure);

	/*!< Send ACMD51 SD_APP_SEND_SCR with argument as 0 */
	SDIO_CmdInitStructure.SDIO_Argument = 0x0;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_APP_SEND_SCR;//ACMD51
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_SD_APP_SEND_SCR);
	if(errorstatus != SD_OK)	return(errorstatus);

	while(!(SDIO->STA & (SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND | SDIO_FLAG_STBITERR)))
	{
		if(SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)//����FIFO���ݿ���
		{
			*(tempscr + index) = SDIO_ReadData();//��ȡFIFO����
			index++;
		}
	}
	if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)//���ݳ�ʱ����
	{
		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
		errorstatus = SD_DATA_TIMEOUT;
		return(errorstatus);
	}
	else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)//���ݿ�CRC����
	{
		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
		errorstatus = SD_DATA_CRC_FAIL;
		return(errorstatus);
	}
	else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)//����fifo�������
	{
		SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
		errorstatus = SD_RX_OVERRUN;
		return(errorstatus);
	}
	else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)//������ʼλ����
	{
		SDIO_ClearFlag(SDIO_FLAG_STBITERR);
		errorstatus = SD_START_BIT_ERR;
		return(errorstatus);
	}
	/*!< Clear all the static flags */
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);
	
	/* ������˳��8λΪ��λ������.*/
	*(pscr+1)= ((tempscr[0]& SD_0TO7BITS) <<24) | ((tempscr[0]& SD_8TO15BITS) <<8) | ((tempscr[0]& SD_16TO23BITS) >>8) | ((tempscr[0]& SD_24TO31BITS) >>24);
	*(pscr)  = ((tempscr[1]& SD_0TO7BITS) <<24) | ((tempscr[1]& SD_8TO15BITS) <<8) | ((tempscr[1]& SD_16TO23BITS) >>8) | ((tempscr[1]& SD_24TO31BITS) >>24);

	return(errorstatus);
}

//����:       ��NumOfBytes�����(��2Ϊ��)
//NumOfBytes: �ֽ���, ��512
//����ֵ:     ��2Ϊ�׵�ָ��ֵ, ��9
u8 convert_from_bytes_to_power_of_two(u16 NumOfBytes)
{
	u8 count = 0;

	while(NumOfBytes != 1)
	{
		NumOfBytes >>= 1;
		count++;
	}
	return(count);
}
