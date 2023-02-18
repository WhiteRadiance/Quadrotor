/*
�ɻ����ֲ�:
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

	1. M1, M3����, ˳ʱ����ת; M2, M4����, ��ʱ����ת
	  ��ע����Ϊ��һ��ɻ��ĵ���������򺸽�, ���µ�һ��ĵ��ת���������պ��෴
	2. X��: 	Y��: 
	3. Pitch:	������, ���� X ����ת
	   Roll:	�����, ���� Y ����ת
	   Yaw:		ƫ����, ���� Z ������
	4. PID����ʱӦ��һ����һ����ĵ���, ����������һ���
*/


#include "PID.h"
#include "led.h"



//TaskMask_s			TaskMask = {0};
SetValue_s			SV = {0};
MeasuredValue_s		MV = {0};
PID_Output_s		PID_out = {0};
volatile int16_t 	MotorPWM[4]={0};	//������PID���ֵ֮��, �����͸������PWMֵ

float e_Integ_X, e_Integ_Y;		//�ڻ�PID��������(�ۼ�)ֵ
float e_Integ_pit, e_Integ_rol;			//�⻷PID��������(�ۼ�)ֵ

extern u16	rx_adc_thrust;
extern short	rx_adc_pitch;
extern short	rx_adc_roll;
extern short	rx_adc_yaw;
extern u8	THR_unlock;

extern float		pitch, roll, yaw;
extern short 		gyro[3], accel[3];

extern u8 rssi;				//ͨѶ�ɹ�һ�ξͻḳֵΪ255, ����᲻���Լ�, ͨ���ж�rssi=0���ж��Ƿ����




//PID����Ԥ��/����ֵ �� ����ֵ��׼������
void PID_prepare()
{
	MV.pit = pitch + 0.50;		//��Ϊʵ�ʷ���ʱ����ȥ��0.50��ͻ���ǰƯ��
	MV.rol = roll + 0.24;		//��Ϊʵ�ʷ���ʱ����ȥ��0.23��ͻ�����Ư��
	MV.yaw = yaw;
	MV.V_Angular_X = gyro[0] * 0.030518f;	//��gyro��ADCֵ����ɽ��ٶȡ�/s
	MV.V_Angular_Y = gyro[1] * 0.030518f;	//2000��/s  /  65535  =  0.030518043793
	
	SV.pit = rx_adc_pitch;
	SV.rol = rx_adc_roll;
//	SV.pit = DataPacket.leftRight - 15;		//����ң���������������ҷ���ֵ, �ı�SV.pit��Ԥ��ֵ
//	SV.rol = DataPacket.upDown - 15;		//����ң������������ǰ����ֵ, �ı�SV.rol��Ԥ��ֵ
	
	//SV.yaw = rx_adc_yaw;					//��Ϊ����ҡ���������֮��Ļ�����������ĳ�Ƕ�, ���Բ���ֱ�Ӹ�yaw��ֵ��0��
}




//�⻷PID���� (��P����) (����ʱ�����΢����I����)
void Outter_PID()
{
	static float Kp1 = 1.9;			//��X��ı���ϵ�� 2  //2.6		//2.0
	static float Kp2 = 2.0;			//��Y��ı���ϵ�� 1  //2.5		//1.9
	
	static float Ki1 = 0.160;		// 0.15		//0.190
	static float Ki2 = 0.170;		//			//0.160
	
	static float Kd1 = 6.60;		//			//6.70
	static float Kd2 = 6.40;		//			//5.90
	
	static float e_pit, e_rol;		//����ƫ��
	static float e_pit_old, e_rol_old;
	
	static u8	 flag_pit, flag_rol;//���ַ���ʱ���ƻ���ϵ���Ƿ��������ı�־
	
	//�����Ƕ�ƫ��
	e_pit = SV.pit - MV.pit;
	e_rol = SV.rol - MV.rol;
	
#if 1
	//============================  �⻷pit  PID����  ================================
	//���ַ���, �Ա���ƫ��ܴ�ʱ��ֹ������, �����ƫ���Сʱ�ż����������
	if((e_pit >= 16.0)||(e_pit <= -16.0))
		flag_pit = 0;				//ƫ��̫��ʱʹ��������ʧЧ
	else{
		flag_pit = 1;				//����ʹ�û�������
		e_Integ_pit += e_pit;		//�ۼ�
	}	
	//�����޷�
	if(e_Integ_pit > 100)		e_Integ_pit = 100;
	if(e_Integ_pit < -100)		e_Integ_pit = -100;
#endif
#if 1
	//============================  �⻷rol  PID����  ================================
	//���ַ���, �Ա���ƫ��ܴ�ʱ��ֹ������, �����ƫ���Сʱ�ż����������
	if((e_rol >= 16.0)||(e_rol <= -16.0))
		flag_rol = 0;				//ƫ��̫��ʱʹ��������ʧЧ
	else{
		flag_rol = 1;				//����ʹ�û�������
		e_Integ_rol += e_rol;		//�ۼ�
	}	
	//�����޷�
	if(e_Integ_rol > 100)		e_Integ_rol = 100;
	if(e_Integ_rol < -100)		e_Integ_rol = -100;
#endif
	
	//�⻷PID��P����
	SV.V_Angular_X = Kp1 * e_pit + flag_pit * Ki1 * e_Integ_pit + Kd1 * (e_pit - e_pit_old);
	SV.V_Angular_Y = Kp2 * e_rol + flag_rol * Ki2 * e_Integ_rol + Kd2 * (e_rol - e_rol_old);
	
	
	//ʱ������, ����ƫ���Ϊǰ��ƫ������ظ�
	e_pit_old = e_pit;
	e_rol_old = e_rol;
}




//�ڻ�PID���� (P,I,D)
void Inner_PID()
{
	static float Kp1=3.04, Ki1=0.105, Kd1=6.40;		//��X���PIDϵ�� 30 0.5 30  //2.35 0.011 1.90	//3.00 0.105 6.90
	static float Kp2=3.07, Ki2=0.102, Kd2=6.25;		//��Y���PIDϵ�� 25 0.5 22  //2.34 0.012 1.91	//3.00 0.102 5.91
	static float e_X_now, e_X_old;					//��X��ı���ƫ���ǰ��ƫ��
	static float e_Y_now, e_Y_old;					//��Y��ı���ƫ���ǰ��ƫ��
	//static float e_Integ_X, e_Integ_Y;			//�ڻ�PID��������(�ۼ�)ֵ
	static u8	 flag_X, flag_Y;					//���ַ���ʱ���ƻ���ϵ���Ƿ��������ı�־
	
	
	//������ٶ�ƫ��
	e_X_now = SV.V_Angular_X - MV.V_Angular_X;
	e_Y_now = SV.V_Angular_Y - MV.V_Angular_Y;
	
	
#if 1
	//============================= �� X ����ڻ�PID���� =================================
	//���ַ���, �Ա���ƫ��ܴ�ʱ��ֹ������, �����ƫ���Сʱ�ż����������
	if((e_X_now >= 110.0)||(e_X_now <= -110.0))
		flag_X = 0;				//ƫ��̫��ʱʹ��������ʧЧ
	else{
		flag_X = 1;				//����ʹ�û�������
		e_Integ_X += e_X_now;	//�ۼ�
	}	
	//�����޷�
	if(e_Integ_X > 400)		e_Integ_X = 400;
	if(e_Integ_X < -400)	e_Integ_X = -400;
	
	//λ��ʽPID����
	PID_out.X = Kp1 * e_X_now + flag_X * Ki1 * e_Integ_X + Kd1 * (e_X_now - e_X_old);
#endif
	
	
#if 1
	//============================= �� Y ����ڻ�PID���� =================================
	//���ַ���, �Ա���ƫ��ܴ�ʱ��ֹ������, �����ƫ���Сʱ�ż����������
	if((e_Y_now >= 110.0)||(e_Y_now <= -110.0))
		flag_Y = 0;				//ƫ��̫��ʱʹ��������ʧЧ
	else{
		flag_Y = 1;				//����ʹ�û�������
		e_Integ_Y += e_Y_now;	//�ۼ�
	}	
	//�����޷�
	if(e_Integ_Y > 400)		e_Integ_Y = 400;
	if(e_Integ_Y < -400)	e_Integ_Y = -400;
	
	//λ��ʽPID����
	PID_out.Y = Kp2 * e_Y_now + flag_Y * Ki2 * e_Integ_Y + Kd2 * (e_Y_now - e_Y_old);
#endif
	
	
	
	//ʱ������, ����ƫ���Ϊǰ��ƫ������ظ�
	e_X_old = e_X_now;			//X��
	e_Y_old = e_Y_now;			//Y��
}



//Yaw�ǵ��Դ��������� (����PD����)
void Yaw_Rotate_PD_Ctrl()
{
	static float Kp1=3.1, Kd1=6.7;			//��Z���PDϵ��		//40, 60	//3.0  6.5
	static float e_Z_now, e_Z_old;			//��Z��ı���ƫ���ǰ��ƫ��
	
	//����Yaw�ǵ�ƫ��
	e_Z_now = SV.yaw - MV.yaw;
	
	//λ��ʽ����PD����
	PID_out.Z = Kp1 * e_Z_now + Kd1 * (e_Z_now - e_Z_old);
	
	//ʱ������, ����ƫ���Ϊǰ��ƫ������ظ�
	e_Z_old = e_Z_now;			//Z��
}




//�ɻ���������ʱ��PWM�Ĵ���
void Motor_PWM_Lock()
{
	/* ���RSSI�ź�ǿ�ȵ���-64dBm */
//	if(si24r1_RX_RSSI()==0)			//ң������RSSI����, ���ǲ�֪��Ϊɶ�ɻ���RSSI��Զ����-64dBm
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
			
			rx_adc_thrust = 0;	//�ر�����
		}
		led1 = 1;
	}
	else
		led1 = 0;
	if(THR_unlock == 0x05)	//����յ�����������
	{
		if((rx_adc_thrust*3/20) >= 796)
			rx_adc_thrust = 5300;		//��ֹPWMռ�ձȳ���100%
		else
			rx_adc_thrust = rx_adc_thrust;
		
		if(rx_adc_thrust <= 700)		//����(0-4095)����700, ����Ϊ�ɻ��ǵ��ٴ���״̬, ��ʱҲ���PID�����
		{
			e_Integ_X = 0;	//������ѻ���ֵ��0���һֱ�ۼ�, Ӱ������Ժ��PID���
			e_Integ_Y = 0;
			PID_out.X = 0;
			PID_out.Y = 0;
			PID_out.Z = 0;
			
			e_Integ_pit = 0;
			e_Integ_rol = 0;
		}
		else if(rx_adc_thrust <= 2300)	//����(0-4095)����2300ʱ�ɻ��ŵ��Ѿ���΢���������
			PID_out.Z = 0;				//�ɻ�û���뿪����ʱ������PID����Yaw��
		else ;
	}
	else					//�������ά������
	{
		//���⻷���ٶ����ֵ��λ��0, ��¼��ǰ��õ�yaw�ǲ����ǳ�Ԥ��yaw��
		SV.yaw = MV.yaw;	//��¼��ǰ��õ�yaw��
		SV.V_Angular_X = 0;
		SV.V_Angular_Y = 0;
		
		//��ƫ���ۼ�ֵ �� PID�������λ��0
		e_Integ_X = 0;	//������ѻ���ֵ��0���һֱ�ۼ�, Ӱ������Ժ��PID���
		e_Integ_Y = 0;
		PID_out.X = 0;
		PID_out.Y = 0;
		PID_out.Z = 0;
		
		e_Integ_pit = 0;
		e_Integ_rol = 0;
		
		//���յ�������ֵ�ĳ�0
		rx_adc_thrust = 0;		
	}
}


//��PID���������������ֵ��
void Set_PWM_plus_PID()
{
	static int16_t max = 799;	//ʵ��PWM��ռ�ձȷ�Χֻ��(0-799)
	static int16_t min = 8;
	u8 i = 0;
	
	//�Բ�ͬ����ĵ�����в�ͬ��PID����(������)
	MotorPWM[0] = rx_adc_thrust*3/20 - PID_out.X + PID_out.Y - PID_out.Z;	//Motor 1
	MotorPWM[1] = rx_adc_thrust*3/20 + PID_out.X + PID_out.Y + PID_out.Z;	//Motor 2
	MotorPWM[2] = rx_adc_thrust*3/20 + PID_out.X - PID_out.Y - PID_out.Z;	//Motor 3
	MotorPWM[3] = rx_adc_thrust*3/20 - PID_out.X - PID_out.Y + PID_out.Z;	//Motor 4
	
	//��PWM�ļ�ֵ��������
	for(i=0; i<4; i++)
	{
		if(MotorPWM[i] >= max)
			MotorPWM[i] = max;
		else if(MotorPWM[i] <= min)	//ͬʱ���Խ�ֹPWM���ָ�ֵ
			MotorPWM[i] = 0;
	}
	
	//��PWMֵװ�䵽���
	TIM_SetCompare1(TIM4, MotorPWM[0]);
	TIM_SetCompare2(TIM4, MotorPWM[1]);
	TIM_SetCompare3(TIM4, MotorPWM[2]);
	TIM_SetCompare4(TIM4, MotorPWM[3]);
}


