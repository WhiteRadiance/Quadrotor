#ifndef _TIMER_H
#define _TIMER_H
#include "sys.h"
/*
/	TIM1, TIM8 为高级定时器, 16位, 相互独立
/	TIM2, TIM3, TIM4, TIM5 为通用定时器, 16位, 相互独立, TIMx有4个独立通道
/	TIM6, TIM7 为基本定时器, 16位, 相互独立, 可以为通用定时器提供时间基准, 可以为DAC提供时钟
*/

//arr: Auto_Reload_Reg	自动重载值
//psc: Pre_scaler		预分频值

void TIM1_PWM_Init(u16 arr, u16 psc);

void TIM3_Config_Init(u16 arr, u16 psc);

#endif
