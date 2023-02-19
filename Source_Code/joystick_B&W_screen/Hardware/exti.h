#ifndef _EXTI_H
#define _EXTI_H

void exti_init(void);//IO初始化


/*
下面这三个中断服务函数被 官方固件库 提前声明过
void EXTI0_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI15_10_IRQHandler(void);
*/

#endif
