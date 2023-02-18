#include "task.h"
#include "mpu_dmp_api.h"
#include "si24r1.h"
#include "PID.h"
#include "led.h"
#include "DataPackage.h"


extern u8	MPU_DMP_RDY;		//MPU_DMP��INT�ų����жϵı�־
extern u8	NRF_IRQ_TRIG;		//NRF_IRQ�����жϵı�־

extern u8	Aircraft_RFbuf[10];
extern u8	TeleCtrl_RFbuf[20];
extern u8	rssi;				//ÿ��ͨѶ�ɹ��͸�ֵΪ255, ����᲻���Լ�, ͨ���ж�rssi=0���ж��Ƿ����




//Task for MPU_DMP_PID_PWM to 4 Motors
void Task_MPU_DMP_PID()
{
	static u8 i =1;	//static����ʹ��NRF�ĳ�ʼ����ִ��һ��
	while(mpu_dmp_getdata() != 0);	//�����ȡFIFOʧ�ܾ����̼�����ȡֱ���ɹ�
	//usart1_report_imu(accel[0],  accel[1],  accel[2],  gyro[0],  gyro[1],  gyro[2],\
						(int16_t)(roll*100),  (int16_t)(pitch*100),  (int16_t)(yaw*10));
	if(i==1)
	{
		si24r1_RX_mode();	//DMP�״ζ�ȡ��Ԫ���������ٳ�ʼ��NRF���Ա�֤: (��һ��)
		NRF_IRQ_Config();	//��ʹң�������ϵ�, �ɻ����ϵ�Ҳ�������״�DMP����
		i--;
	}
	else
		i = 0;
	
	PID_prepare();
	
	//PID����
	Outter_PID();
	Inner_PID();
	Yaw_Rotate_PD_Ctrl();
	
	Motor_PWM_Lock();	//���ɻ��ļ������� -> �Ƿ�ر�����	
	Set_PWM_plus_PID();	//��PID�����������䵽4�����
	
	
	MPU_DMP_RDY = 0;	//�˳�����
}




//Task for NRF_RX for Dupex_Telecom
void Task_NRF_TeleCom()
{	
	if(si24r1_RX_packet(TeleCtrl_RFbuf, Aircraft_RFbuf) & RX_OK)
	{
		rssi = 255;
		
		led2 = 0;
				
		/* �Խ��ܵ����ݽ��в�� */
		Data_UnPack_from_RX();
	
		/* �Իش������ݽ��д�� */
		Data_Package_to_TX();
	}
	else
		led2 = 1;

	
	NRF_IRQ_TRIG = 0;	//�˳�����
}	



