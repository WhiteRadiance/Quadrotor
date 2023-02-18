#ifndef _PID_H
#define _PID_H

#include "stm32f10x.h"


//指示哪些任务是否开启的结构体 (位域格式, 节省内存)
typedef struct _TaskMask_s_{
	u16	check_NRF:			1;	//第一位
	u16	angleDataReady:		1;	//第二位
	u16 nrfDataReady:		1;	//第三位
	
	u16 reserved:			13;	//保留项
}TaskMask_s;



//PID给定/预期值结构体
typedef struct _SetValue_s_{
	volatile float pit;			//pitch角的预期值
	volatile float rol;			//roll角的预期值
	volatile float yaw;			//yaw角的预期值
	volatile float V_Angular_X;	//X轴角速度
	volatile float V_Angular_Y;	//Y轴角速度
}SetValue_s;



//MPU6050测量值结构体
typedef struct _MeasuredValue_s_{
	float pit;					//pitch角的预期值
	float rol;					//roll角的预期值
	float yaw;					//yaw角的预期值
	float V_Angular_X;			//X轴角速度
	float V_Angular_Y;			//Y轴角速度
}MeasuredValue_s;



//PID系统输出的PWM补偿值
typedef struct _PID_Output_s_{
	volatile float X;			//内环PID运算后输出的X轴补偿值
	volatile float Y;			//内环PID运算后输出的Y轴补偿值
	volatile float Z;			//偏航角Yaw的补偿值
}PID_Output_s;





void PID_prepare(void);
void Outter_PID(void);
void Inner_PID(void);
void Yaw_Rotate_PD_Ctrl(void);
void Motor_PWM_Lock(void);
void Set_PWM_plus_PID(void);


#endif
