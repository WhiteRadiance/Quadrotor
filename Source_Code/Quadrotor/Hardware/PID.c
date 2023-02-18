/*
飞机马达分布:
				Y+
		  M4	|    M1
			\   |   /
			 \  |  /
			  \ | /
		 -------+-------> X+	
			  / | \
			 /  |  \
			/   |   \
		  M3    |    M2

	1. M1, M3正接, 顺时针旋转; M2, M4反接, 逆时针旋转
	  【注】因为第一版飞机的电机插座反向焊接, 导致第一版的电机转向与上述刚好相反
	2. X轴: 	Y轴: 
	3. Pitch:	俯仰角, 即绕 X 轴旋转
	   Roll:	横滚角, 即绕 Y 轴旋转
	   Yaw:		偏航角, 即绕 Z 轴自旋
	4. PID调参时应该一个轴一个轴的调试, 切勿三个轴一起搞
*/


#include "PID.h"
#include "led.h"



//TaskMask_s			TaskMask = {0};
SetValue_s			SV = {0};
MeasuredValue_s		MV = {0};
PID_Output_s		PID_out = {0};
volatile int16_t 	MotorPWM[4]={0};	//补偿过PID输出值之后, 最终送给电机的PWM值

float e_Integ_X, e_Integ_Y;		//内环PID的误差积分(累加)值
float e_Integ_pit, e_Integ_rol;			//外环PID的误差积分(累加)值

extern u16	rx_adc_thrust;
extern short	rx_adc_pitch;
extern short	rx_adc_roll;
extern short	rx_adc_yaw;
extern u8	THR_unlock;

extern float		pitch, roll, yaw;
extern short 		gyro[3], accel[3];

extern u8 rssi;				//通讯成功一次就会赋值为255, 程序会不断自减, 通过判断rssi=0来判断是否断联




//PID各个预期/给定值 和 测量值的准备工作
void PID_prepare()
{
	MV.pit = pitch + 0.50;		//因为实际飞行时不减去这0.50°就会向前漂移
	MV.rol = roll + 0.24;		//因为实际飞行时不减去这0.23°就会向左漂移
	MV.yaw = yaw;
	MV.V_Angular_X = gyro[0] * 0.030518f;	//将gyro的ADC值换算成角速度°/s
	MV.V_Angular_Y = gyro[1] * 0.030518f;	//2000°/s  /  65535  =  0.030518043793
	
	SV.pit = rx_adc_pitch;
	SV.rol = rx_adc_roll;
//	SV.pit = DataPacket.leftRight - 15;		//根据遥控器传过来的左右方向值, 改变SV.pit的预期值
//	SV.rol = DataPacket.upDown - 15;		//根据遥控器传过来的前后方向值, 改变SV.rol的预期值
	
	//SV.yaw = rx_adc_yaw;					//因为自旋摇杆是在起飞之后的基础上再自旋某角度, 所以不能直接给yaw赋值成0度
}




//外环PID控制 (单P控制) (测试时另加了微弱的I控制)
void Outter_PID()
{
	static float Kp1 = 1.9;			//绕X轴的比例系数 2  //2.6		//2.0
	static float Kp2 = 2.0;			//绕Y轴的比例系数 1  //2.5		//1.9
	
	static float Ki1 = 0.160;		// 0.15		//0.190
	static float Ki2 = 0.170;		//			//0.160
	
	static float Kd1 = 6.60;		//			//6.70
	static float Kd2 = 6.40;		//			//5.90
	
	static float e_pit, e_rol;		//本次偏差
	static float e_pit_old, e_rol_old;
	
	static u8	 flag_pit, flag_rol;//积分分离时控制积分系数是否参与运算的标志
	
	//计数角度偏差
	e_pit = SV.pit - MV.pit;
	e_rol = SV.rol - MV.rol;
	
#if 1
	//============================  外环pit  PID运算  ================================
	//积分分离, 以便在偏差很大时防止产生振荡, 因此在偏差较小时才加入积分作用
	if((e_pit >= 16.0)||(e_pit <= -16.0))
		flag_pit = 0;				//偏差太大时使积分作用失效
	else{
		flag_pit = 1;				//允许使用积分作用
		e_Integ_pit += e_pit;		//累加
	}	
	//积分限幅
	if(e_Integ_pit > 100)		e_Integ_pit = 100;
	if(e_Integ_pit < -100)		e_Integ_pit = -100;
#endif
#if 1
	//============================  外环rol  PID运算  ================================
	//积分分离, 以便在偏差很大时防止产生振荡, 因此在偏差较小时才加入积分作用
	if((e_rol >= 16.0)||(e_rol <= -16.0))
		flag_rol = 0;				//偏差太大时使积分作用失效
	else{
		flag_rol = 1;				//允许使用积分作用
		e_Integ_rol += e_rol;		//累加
	}	
	//积分限幅
	if(e_Integ_rol > 100)		e_Integ_rol = 100;
	if(e_Integ_rol < -100)		e_Integ_rol = -100;
#endif
	
	//外环PID单P运算
	SV.V_Angular_X = Kp1 * e_pit + flag_pit * Ki1 * e_Integ_pit + Kd1 * (e_pit - e_pit_old);
	SV.V_Angular_Y = Kp2 * e_rol + flag_rol * Ki2 * e_Integ_rol + Kd2 * (e_rol - e_rol_old);
	
	
	//时间流逝, 本次偏差成为前次偏差并不断重复
	e_pit_old = e_pit;
	e_rol_old = e_rol;
}




//内环PID控制 (P,I,D)
void Inner_PID()
{
	static float Kp1=3.04, Ki1=0.105, Kd1=6.40;		//绕X轴的PID系数 30 0.5 30  //2.35 0.011 1.90	//3.00 0.105 6.90
	static float Kp2=3.07, Ki2=0.102, Kd2=6.25;		//绕Y轴的PID系数 25 0.5 22  //2.34 0.012 1.91	//3.00 0.102 5.91
	static float e_X_now, e_X_old;					//绕X轴的本次偏差和前次偏差
	static float e_Y_now, e_Y_old;					//绕Y轴的本次偏差和前次偏差
	//static float e_Integ_X, e_Integ_Y;			//内环PID的误差积分(累加)值
	static u8	 flag_X, flag_Y;					//积分分离时控制积分系数是否参与运算的标志
	
	
	//计算角速度偏差
	e_X_now = SV.V_Angular_X - MV.V_Angular_X;
	e_Y_now = SV.V_Angular_Y - MV.V_Angular_Y;
	
	
#if 1
	//============================= 绕 X 轴的内环PID运算 =================================
	//积分分离, 以便在偏差很大时防止产生振荡, 因此在偏差较小时才加入积分作用
	if((e_X_now >= 110.0)||(e_X_now <= -110.0))
		flag_X = 0;				//偏差太大时使积分作用失效
	else{
		flag_X = 1;				//允许使用积分作用
		e_Integ_X += e_X_now;	//累加
	}	
	//积分限幅
	if(e_Integ_X > 400)		e_Integ_X = 400;
	if(e_Integ_X < -400)	e_Integ_X = -400;
	
	//位置式PID运算
	PID_out.X = Kp1 * e_X_now + flag_X * Ki1 * e_Integ_X + Kd1 * (e_X_now - e_X_old);
#endif
	
	
#if 1
	//============================= 绕 Y 轴的内环PID运算 =================================
	//积分分离, 以便在偏差很大时防止产生振荡, 因此在偏差较小时才加入积分作用
	if((e_Y_now >= 110.0)||(e_Y_now <= -110.0))
		flag_Y = 0;				//偏差太大时使积分作用失效
	else{
		flag_Y = 1;				//允许使用积分作用
		e_Integ_Y += e_Y_now;	//累加
	}	
	//积分限幅
	if(e_Integ_Y > 400)		e_Integ_Y = 400;
	if(e_Integ_Y < -400)	e_Integ_Y = -400;
	
	//位置式PID运算
	PID_out.Y = Kp2 * e_Y_now + flag_Y * Ki2 * e_Integ_Y + Kd2 * (e_Y_now - e_Y_old);
#endif
	
	
	
	//时间流逝, 本次偏差成为前次偏差并不断重复
	e_X_old = e_X_now;			//X轴
	e_Y_old = e_Y_now;			//Y轴
}



//Yaw角的自传修正补偿 (单环PD控制)
void Yaw_Rotate_PD_Ctrl()
{
	static float Kp1=3.1, Kd1=6.7;			//绕Z轴的PD系数		//40, 60	//3.0  6.5
	static float e_Z_now, e_Z_old;			//绕Z轴的本次偏差和前次偏差
	
	//计算Yaw角的偏差
	e_Z_now = SV.yaw - MV.yaw;
	
	//位置式单环PD运算
	PID_out.Z = Kp1 * e_Z_now + Kd1 * (e_Z_now - e_Z_old);
	
	//时间流逝, 本次偏差成为前次偏差并不断重复
	e_Z_old = e_Z_now;			//Z轴
}




//飞机加锁解锁时对PWM的处理
void Motor_PWM_Lock()
{
	/* 如果RSSI信号强度低于-64dBm */
//	if(si24r1_RX_RSSI()==0)			//遥控器的RSSI正常, 但是不知道为啥飞机的RSSI永远低于-64dBm
	if(rssi == 0)
	{
		//SV.yaw = MV.yaw;
		SV.V_Angular_X = 0;
		SV.V_Angular_Y = 0;
		
		if(rx_adc_thrust >= 600)
			rx_adc_thrust -= 50;
		else
		{
			e_Integ_X = 0;	e_Integ_Y = 0;
			PID_out.X = 0;	PID_out.Y = 0;	PID_out.Z = 0;		
			e_Integ_pit=0;	e_Integ_rol=0;
			
			rx_adc_thrust = 0;	//关闭油门
		}
		led1 = 1;
	}
	else
		led1 = 0;
	if(THR_unlock == 0x05)	//如果收到解锁的命令
	{
		if((rx_adc_thrust*3/20) >= 796)
			rx_adc_thrust = 5300;		//防止PWM占空比超过100%
		else
			rx_adc_thrust = rx_adc_thrust;
		
		if(rx_adc_thrust <= 700)		//油门(0-4095)低于700, 则认为飞机是低速待飞状态, 此时也清空PID的输出
		{
			e_Integ_X = 0;	//如果不把积分值清0则会一直累加, 影响起飞以后的PID输出
			e_Integ_Y = 0;
			PID_out.X = 0;
			PID_out.Y = 0;
			PID_out.Z = 0;
			
			e_Integ_pit = 0;
			e_Integ_rol = 0;
		}
		else if(rx_adc_thrust <= 2300)	//油门(0-4095)高于2300时飞机脚垫已经轻微拖离地面了
			PID_out.Z = 0;				//飞机没有离开地面时不允许PID控制Yaw角
		else ;
	}
	else					//其余命令都维持锁定
	{
		//将外环角速度输出值复位成0, 记录当前测得的yaw角并覆盖成预期yaw角
		SV.yaw = MV.yaw;	//记录当前测得的yaw角
		SV.V_Angular_X = 0;
		SV.V_Angular_Y = 0;
		
		//将偏差累加值 和 PID输出都复位成0
		e_Integ_X = 0;	//如果不把积分值清0则会一直累加, 影响起飞以后的PID输出
		e_Integ_Y = 0;
		PID_out.X = 0;
		PID_out.Y = 0;
		PID_out.Z = 0;
		
		e_Integ_pit = 0;
		e_Integ_rol = 0;
		
		//将收到的油门值改成0
		rx_adc_thrust = 0;		
	}
}


//将PID的输出补偿到油门值上
void Set_PWM_plus_PID()
{
	static int16_t max = 799;	//实际PWM的占空比范围只有(0-799)
	static int16_t min = 8;
	u8 i = 0;
	
	//对不同分组的电机进行不同的PID补偿(正负号)
	MotorPWM[0] = rx_adc_thrust*3/20 - PID_out.X + PID_out.Y - PID_out.Z;	//Motor 1
	MotorPWM[1] = rx_adc_thrust*3/20 + PID_out.X + PID_out.Y + PID_out.Z;	//Motor 2
	MotorPWM[2] = rx_adc_thrust*3/20 + PID_out.X - PID_out.Y - PID_out.Z;	//Motor 3
	MotorPWM[3] = rx_adc_thrust*3/20 - PID_out.X - PID_out.Y + PID_out.Z;	//Motor 4
	
	//对PWM的极值进行限制
	for(i=0; i<4; i++)
	{
		if(MotorPWM[i] >= max)
			MotorPWM[i] = max;
		else if(MotorPWM[i] <= min)	//同时可以禁止PWM出现负值
			MotorPWM[i] = 0;
	}
	
	//将PWM值装配到电机
	TIM_SetCompare1(TIM4, MotorPWM[0]);
	TIM_SetCompare2(TIM4, MotorPWM[1]);
	TIM_SetCompare3(TIM4, MotorPWM[2]);
	TIM_SetCompare4(TIM4, MotorPWM[3]);
}


