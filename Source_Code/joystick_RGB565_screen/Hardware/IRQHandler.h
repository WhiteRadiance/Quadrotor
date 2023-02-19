#ifndef _IRQHANDLAER_H
#define _IRQHANDLAER_H

#include "stm32f10x.h"

#define USB_MOUSE
#if defined USB_MOUSE
	#include "platform_config.h"
#endif

void SDIO_IRQHandler(void);
void TIM3_IRQHandler(void);
void DMAChannel3_IRQHandler(void);

void EXTI15_10_IRQHandler(void);
void EXTI9_5_IRQHandler(void);

//IRQ For USB
void USBWakeUp_IRQHandler(void);
void /*USB_LP_IRQHandler*/USB_LP_CAN1_RX0_IRQHandler(void);

#endif
