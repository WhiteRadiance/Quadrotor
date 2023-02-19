#include "IRQHandler.h"
#include "delay.h"
#include "key.h"
#include "spi.h"
#include "dma.h"
#include "sdio_sdcard.h"

//For USB
#include "usb_istr.h"
#include "usb_lib.h"
#include "usb_pwr.h"
#include "platform_config.h"


extern u8	Trig_NRF_TX;	//����һ�����ݷ���
extern u8	Trig_LED_Scan;	//�Թ���ָʾ��ÿ2Hzȡ��
extern u8	Trig_oled_FPS;	//��ͨ��ʾ�����fps = 25Hz
extern u8	Trig_adc_bat;	//���ﲻ�Ǵ���ADCת��, ����10Hz��������ǰADC��ֵ���ӵ�adc_battery


extern u8	left_key_value;
extern u8 	right_key_value;
extern u8	user1_key_value;
extern u8	user2_key_value;


/******************************************************************************/
/*                 STM32 Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32xxx.s).                                            */
/******************************************************************************/

/* USB Interrupt */

/*******************************************************************************
* Function Name  : USBWakeUp_IRQHandler
* Description    : This function handles USB WakeUp interrupt request.
* Return         : None
*******************************************************************************/
void USBWakeUp_IRQHandler(void)//USB�����жϷ�����
{
	EXTI_ClearITPendingBit(EXTI_Line18);//���USB�����жϹ���λ
}


/*******************************************************************************
* Function Name  : USB_IRQHandler
* Description    : This function handles USB Low Priority interrupts
*                  requests.
* Return         : None
*******************************************************************************/
//void USB_LP_IRQHandler(void)//USB�жϴ�����
void USB_LP_CAN1_RX0_IRQHandler(void)
{
	USB_Istr();
}






//SDIO�жϷ�����
void SDIO_IRQHandler(void)
{
	/* Process All SDIO Interrupt Sources */
	SD_ProcessIRQSrc();
}


//TIM3 �жϷ������
void TIM3_IRQHandler(void)//1ms����
{
	static u16 ms50=0, ms500=0, ms40=0, ms100=0;
	
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)	//���ָ����TIM�жϷ������
	{
		ms50++;  ms500++;  ms40++;  ms100++;
		
		if(ms50 >= 50){		// 20Hz
			ms50 = 0;
			Trig_NRF_TX = 1;
		}
		else ;
		if(ms500 >= 500){	// 2Hz
			ms500 = 0;
			Trig_LED_Scan = 1;
		}
		else ;
		if(ms40 >= 40){		// 25Hz
			ms40 = 0;
			Trig_oled_FPS = 1;
		}
		else ;
		if(ms100 >= 100){	// 10Hz
			ms100 = 0;
			Trig_adc_bat = 1;
		}
		else ;
	}
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);			//���TIM3���жϴ�����λ
}



//SPI1_Tx_DMA���жϴ�����
void DMAChannel3_IRQHandler(void)
{	
	DMA_ClearITPendingBit(DMA1_FLAG_TC3 /*| DMA1_FLAG_TE3 | DMA1_FLAG_GL3*/ );	//����������TC�ı�־
	DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, DISABLE);					//�ر�DMA_TC�ж�
	DMA_Cmd(DMA1_Channel3, DISABLE);									//�ر�DMA����
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
}




/* -------------- �ⲿ�ж� -------------- �ⲿ�ж� -------------- �ⲿ�ж� --------------- �ⲿ�ж� ------------- */


//�жϷ�����
void EXTI9_5_IRQHandler(void)					// key1(PB8)
{
	if(EXTI_GetITStatus(EXTI_Line8)!=RESET)
	{
		delay_ms(7);//����
		if(key1==0)	//�ٴ��ж�(�͵�ƽ)---���ڷǳ�����!
			left_key_value += 1;
		else ;
		if(left_key_value == 2)
			left_key_value = 0;
		else ;
	}
	EXTI_ClearITPendingBit(EXTI_Line8);
}


void EXTI15_10_IRQHandler(void)					// key2(PB11), key4_user2(PB10)
{
	if(EXTI_GetITStatus(EXTI_Line11)!=RESET)	//�ж�PB11
	{
		delay_ms(7);//����
		if(key2==0)	//�ٴ��ж�(�͵�ƽ)---���ڷǳ�����!
			right_key_value = ~right_key_value;	//0x00, 0xFF
		else ;
	}
//	if(EXTI_GetITStatus(EXTI_Line11)!=RESET)	//�ж�PB11
//	{
//		delay_ms(7);//����
//		if(key2==0)	//�ٴ��ж�(�͵�ƽ)---���ڷǳ�����!
//			right_key_value += 1;
//		else ;
//		if(right_key_value == 2)
//			right_key_value = 0;
//		else ;
//	}
	else if(EXTI_GetITStatus(EXTI_Line10)!=RESET)//�ж�PB10
	{
		delay_ms(7);//����
		if(key4==0)	//�ٴ��ж�(�͵�ƽ)---���ڷǳ�����!
			user2_key_value = ~user2_key_value;	//0x00, 0xFF
		else ;
	}
	
	EXTI_ClearITPendingBit(EXTI_Line11);
	EXTI_ClearITPendingBit(EXTI_Line10);
}



void EXTI3_IRQHandler(void)						// key3_user1(PC3)
{
	if(EXTI_GetITStatus(EXTI_Line3)!=RESET)
	{
		delay_ms(7);//����
		if(key3==0)	//�ٴ��ж�(�͵�ƽ)---���ڷǳ�����!
			user1_key_value += 1;
		else ;
		if(user1_key_value == 2)
			user1_key_value = 0;
		else ;
	}
	EXTI_ClearITPendingBit(EXTI_Line3);
}
