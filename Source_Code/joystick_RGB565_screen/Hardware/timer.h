#ifndef _TIMER_H
#define _TIMER_H
#include "sys.h"
/*
/	TIM1, TIM8 Ϊ�߼���ʱ��, 16λ, �໥����
/	TIM2, TIM3, TIM4, TIM5 Ϊͨ�ö�ʱ��, 16λ, �໥����, TIMx��4������ͨ��
/	TIM6, TIM7 Ϊ������ʱ��, 16λ, �໥����, ����Ϊͨ�ö�ʱ���ṩʱ���׼, ����ΪDAC�ṩʱ��
*/

//arr: Auto_Reload_Reg	�Զ�����ֵ
//psc: Pre_scaler		Ԥ��Ƶֵ

void TIM1_PWM_Init(u16 arr, u16 psc);

void TIM3_Config_Init(u16 arr, u16 psc);

#endif
