#include "task.h"
#include "mpu_dmp_api.h"
#include "si24r1.h"
#include "PID.h"
#include "led.h"
#include "DataPackage.h"


extern u8	MPU_DMP_RDY;		//MPU_DMP的INT脚出现中断的标志
extern u8	NRF_IRQ_TRIG;		//NRF_IRQ出现中断的标志

extern u8	Aircraft_RFbuf[10];
extern u8	TeleCtrl_RFbuf[20];
extern u8	rssi;				//每次通讯成功就赋值为255, 程序会不断自减, 通过判断rssi=0来判断是否断联




//Task for MPU_DMP_PID_PWM to 4 Motors
void Task_MPU_DMP_PID()
{
	static u8 i =1;	//static声明使得NRF的初始化仅执行一次
	while(mpu_dmp_getdata() != 0);	//如果读取FIFO失败就立刻继续读取直至成功
	//usart1_report_imu(accel[0],  accel[1],  accel[2],  gyro[0],  gyro[1],  gyro[2],\
						(int16_t)(roll*100),  (int16_t)(pitch*100),  (int16_t)(yaw*10));
	if(i==1)
	{
		si24r1_RX_mode();	//DMP首次读取四元数结束后再初始化NRF可以保证: (下一行)
		NRF_IRQ_Config();	//即使遥控器先上电, 飞机后上电也不会打断首次DMP解算
		i--;
	}
	else
		i = 0;
	
	PID_prepare();
	
	//PID运算
	Outter_PID();
	Inner_PID();
	Yaw_Rotate_PD_Ctrl();
	
	Motor_PWM_Lock();	//检查飞机的加锁解锁 -> 是否关闭油门	
	Set_PWM_plus_PID();	//将PID的输出按组分配到4个电机
	
	
	MPU_DMP_RDY = 0;	//退出任务
}




//Task for NRF_RX for Dupex_Telecom
void Task_NRF_TeleCom()
{	
	if(si24r1_RX_packet(TeleCtrl_RFbuf, Aircraft_RFbuf) & RX_OK)
	{
		rssi = 255;
		
		led2 = 0;
				
		/* 对接受的数据进行拆包 */
		Data_UnPack_from_RX();
	
		/* 对回传的数据进行打包 */
		Data_Package_to_TX();
	}
	else
		led2 = 1;

	
	NRF_IRQ_TRIG = 0;	//退出任务
}	



