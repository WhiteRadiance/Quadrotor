/**
  ******************************************************************************
  * @file    hw_config.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Hardware Configuration & Setup
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"

#include "usb_istr.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

//�����ṹ��û�õ�
//ErrorStatus HSEStartUpStatus;
//EXTI_InitTypeDef EXTI_InitStructure;

/* Extern variables ----------------------------------------------------------*/
//USB�����Ƿ����ڽ��еı�־(�������ʵ��û�õ�)
/*extern*/ __IO uint8_t PrevXferComplete = 1;//�����ڹٷ����ӵ�main.c�ж���

/* Private function prototypes -----------------------------------------------*/
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : Set_System
* Description    : Configures Main system clocks & power.
* Input          : None.
* Return         : None.
*******************************************************************************/
//void Set_System(void)
//{
//
//}


/*******************************************************************************
* Function Name  : Set_USBClock
* Description    : Configures USB Clock input (48MHz).
* Return         : None.
*******************************************************************************/
void Set_USBClock(void)//����USBʱ�����ú���, USB_clk=48MHz @ HCLK=72MHz
{
  /* Select USBCLK source */
  RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);		//USB_clk=PLL_clk/1.5=72MHz/1.5=48Mhz
  /* Enable the USB clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);	//USBʱ��ʹ��
}

/*******************************************************************************
* Function Name  : GPIO_AINConfig
* Description    : Configures all IOs as AIN to reduce the power consumption.
* Return         : None.
*******************************************************************************/
//void GPIO_AINConfig(void)
//{
//
//}

/*******************************************************************************
* Function Name  : Enter_LowPowerMode.
* Description    : Power-off system clocks and power while entering suspend mode.
* Return         : None.
*******************************************************************************/
void Enter_LowPowerMode(void)
{
	//printf("USB enter low power mode.\n");
	
	/* Set the device state to suspend */
	bDeviceState = SUSPENDED;	//bDviceState��¼USB������״̬ (��usb_pwr.c���涨��)

	/* Request to enter STOP mode with regulator in low power mode */
	//PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
}

/*******************************************************************************
* Function Name  : Leave_LowPowerMode.
* Description    : Restores system clocks and power while exiting suspend mode.
* Return         : None.
*******************************************************************************/
void Leave_LowPowerMode(void)
{
	DEVICE_INFO *pInfo = &Device_Info;
	//printf("USB leave low power mode.\n");
	/* Set the device state to the correct state */
	if(pInfo->Current_Configuration != 0)
	{
		/* Device configured */
		bDeviceState = CONFIGURED;
	}
	else
	{
		bDeviceState = ATTACHED;
	}
  
	/*Enable SystemCoreClock*/
	//SystemInit();
}

/*******************************************************************************
* Function Name  : USB_Interrupts_Config.
* Description    : Configures the USB interrupts.
* Return         : None.
*******************************************************************************/
void USB_Interrupts_Config(void)
{
	//ע��stm32f103��USB����һ������, PA11/PA12�Զ�����ΪUSB_D+/D-��ʧȥ��ͨIO�Ĺ���
	
	//GPIO_InitTypeDef  GPIO_InitStructure;//��������
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Configure the EXTI line 18 connected internally to the USB IP */
	EXTI_ClearITPendingBit(EXTI_Line18);
	EXTI_InitStructure.EXTI_Line = EXTI_Line18;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;		//������
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* 2 bit for pre-emption priority, 2 bits for subpriority */
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	/* Enable the USB interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//�����ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable the USB Wake-up interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USBWakeUp_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//��ռ���ȼ�1
	NVIC_Init(&NVIC_InitStructure);
}

/*******************************************************************************
* Function Name  : USB_Cable_Config.
* Description    : Software Connection/Disconnection of USB Cable.
* Input          : NewState: new state.
* Return         : None
*******************************************************************************/
//USB�ӿ�����(����1.5K��������,ALIENTEK��M3ϵ�п����壬����Ҫ����,�̶�������������)
//NewState:DISABLE,��������ENABLE,����
void USB_Cable_Config(FunctionalState NewState)
{
	if(NewState != DISABLE);
//	{
//		GPIO_ResetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
//	}
	else;
//	{
//		GPIO_SetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
//	}
}

//USBʹ������/�Ͽ�(����������Cable_Config)   enable:0,�Ͽ�  1,��������
void USB_Port_Set(uint8_t enable)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//ʹ��PortAʱ��
	if(enable)
		_SetCNTR(_GetCNTR()&(~(1<<1)));	//�˳��ϵ�ģʽ
	else{
		_SetCNTR(_GetCNTR()|(1<<1));	//�ϵ�ģʽ
		GPIOA->CRH &= 0xFFF00FFF;
		GPIOA->CRH |= 0x00033000;
		PAout(12) = 0;
	}
}

/*******************************************************************************
* Function Name : Joystick_Send.
* Description   : prepares buffer to be sent containing Joystick event infos.
* Input         : Keys: keys received from terminal.
* Return value  : None.
*******************************************************************************/
void Joystick_Send(u8 key, char ar, char ap, char at)//(uint8_t Keys)
{
	uint8_t Mouse_Buffer[4] = {0, 0, 0, 0};
	//int8_t X = 0, Y = 0;

	/* prepare buffer to send */
	Mouse_Buffer[0] = key;
	Mouse_Buffer[1] = ar;
	Mouse_Buffer[2] = ap;
	Mouse_Buffer[3] = at;

	/* Reset the control token to inform upper layer that a transfer is ongoing */
//	PrevXferComplete = 0;
	
	/* Copy mouse position info in ENDP1 Tx Packet Memory Area*/
	USB_SIL_Write(EP1_IN, Mouse_Buffer, 4);
	/* Enable endpoint for transmission */
	SetEPTxValid(ENDP1);
}


/*******************************************************************************
* Function Name  : Get_SerialNum.
* Description    : Create the serial number string descriptor.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Get_SerialNum(void)
{
  uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

  Device_Serial0 = *(uint32_t*)ID1;
  Device_Serial1 = *(uint32_t*)ID2;
  Device_Serial2 = *(uint32_t*)ID3;
  
  Device_Serial0 += Device_Serial2;

  if (Device_Serial0 != 0)
  {
    IntToUnicode (Device_Serial0, &Joystick_StringSerial[2] , 8);
    IntToUnicode (Device_Serial1, &Joystick_StringSerial[18], 4);
  }
}

/*******************************************************************************
* Function Name  : HexToChar.
* Description    : Convert Hex 32Bits value into char.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len)
{
  uint8_t idx = 0;
  
  for( idx = 0 ; idx < len ; idx ++)
  {
    if( ((value >> 28)) < 0xA )
    {
      pbuf[ 2* idx] = (value >> 28) + '0';
    }
    else
    {
      pbuf[2* idx] = (value >> 28) + 'A' - 10; 
    }
    
    value = value << 4;
    
    pbuf[ 2* idx + 1] = 0;
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
