#ifndef _PID_H
#define _PID_H

#include "stm32f10x.h"


//ָʾ��Щ�����Ƿ����Ľṹ�� (λ���ʽ, ��ʡ�ڴ�)
typedef struct _TaskMask_s_{
	u16	check_NRF:			1;	//��һλ
	u16	angleDataReady:		1;	//�ڶ�λ
	u16 nrfDataReady:		1;	//����λ
	
	u16 reserved:			13;	//������
}TaskMask_s;



//PID����/Ԥ��ֵ�ṹ��
typedef struct _SetValue_s_{
	volatile float pit;			//pitch�ǵ�Ԥ��ֵ
	volatile float rol;			//roll�ǵ�Ԥ��ֵ
	volatile float yaw;			//yaw�ǵ�Ԥ��ֵ
	volatile float V_Angular_X;	//X����ٶ�
	volatile float V_Angular_Y;	//Y����ٶ�
}SetValue_s;



//MPU6050����ֵ�ṹ��
typedef struct _MeasuredValue_s_{
	float pit;					//pitch�ǵ�Ԥ��ֵ
	float rol;					//roll�ǵ�Ԥ��ֵ
	float yaw;					//yaw�ǵ�Ԥ��ֵ
	float V_Angular_X;			//X����ٶ�
	float V_Angular_Y;			//Y����ٶ�
}MeasuredValue_s;



//PIDϵͳ�����PWM����ֵ
typedef struct _PID_Output_s_{
	volatile float X;			//�ڻ�PID����������X�Ჹ��ֵ
	volatile float Y;			//�ڻ�PID����������Y�Ჹ��ֵ
	volatile float Z;			//ƫ����Yaw�Ĳ���ֵ
}PID_Output_s;





void PID_prepare(void);
void Outter_PID(void);
void Inner_PID(void);
void Yaw_Rotate_PD_Ctrl(void);
void Motor_PWM_Lock(void);
void Set_PWM_plus_PID(void);


#endif
