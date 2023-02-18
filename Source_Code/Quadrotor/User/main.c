#include "led.h"
#include "key.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
//#include "oled.h"
#include "exti.h"
//#include "dma.h"
#include "adc.h"
#include "si24r1.h"
#include "timer.h"
#include "i2c.h"
//#include "mpu6050.h"
#include "mpu_dmp_api.h"
#include "task.h"



u8	rssi = 0;	//ÿ���յ�������ֵΪ255���Լ�, ����Լ���0������׶���
u8	Aircraft_RFbuf[10];
u8	TeleCtrl_RFbuf[20];

u8	key1_value = 0;
u16 adc_battery = 0;

extern u8	MPU_DMP_RDY;
extern u8	NRF_IRQ_TRIG;

//extern float		mpu_temperature;
//extern uint8_t	mpu6050_ID;
//extern int16_t	MPU_TEMP;
//extern INT16_XYZ	ACCEL_RAW, GYRO_RAW;
//extern FLOAT_XYZ	Gyro_degree, Gyro_rad;
//extern FLOAT_XYZ	/*Gyro_filt, */Accel_filt;
//extern FLOAT_ANGLE	Angle;
//extern float		mpu_temperature;
extern float		pitch, roll, yaw;
extern short 		gyro[3], accel[3];

u8	Trig_LED_Scan = 0;		// 2Hz
u8	Trig_ADC_Battery = 0;	// 4Hz


int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����2(2bit��ռ��,2bit��Ӧ��)
	delay_init();
	uart_init(115200);
	exti_init();//�˺����ڲ�������key_init()
	led_init();
	adc1_init();

	si24r1_spi_init();
	
	TIM3_Config_Init(99, 719);	//������Ƶ��Ϊ72/(719+1)=100KHz, ���Ƶ��=100K/(99+1)=1KHz, ��1ms���жϴ���
	TIM4_PWM_Init(799,9);		//������Ƶ��Ϊ72/(9+1)=7.2MHz, PWMƵ��=7200/(799+1)=9Khz

	
//	si24r1_RX_mode();	//NRF�ĳ�ʼ��������Task_MPU_DMP_PID()������, �ҽ�ִ��һ�γ�ʼ��
//	NRF_IRQ_Config();	//ͬ�ϡ����������ڶ�ȡ��Ԫ��֮���Ƿ�ֹDMP���״ζ�ȡ��NRFͨ�Ŵ��
	
//	MPU6050_Init();		//�˾�������ڻ����˲�������̬ʱ�ĳ�ʼ��, ��ʹ��DMP�Ͳ�Ҫ����
	IIC_Init();
		
	mpu_dmp_init();		//��ʼ��InvenSense_DMP����
	MPU_INT_Config();	//����MPU_INT���ⲿ�ж�(PA2)

	
	while(1)
	{
		if(Trig_ADC_Battery)
		{
			Trig_ADC_Battery = 0;
			adc_battery = get_once_ADC();
		}
		else ;
			
		
		if(rssi > 0)
			rssi--;
		else
			rssi = 0;


		
		/* Task for MPU_DMP_PID_PWM to 4 Motors */
		if(MPU_DMP_RDY)
			Task_MPU_DMP_PID();
		else ;
		
		
		/* Task for NRF_RX for Dupex_Telecom */
		if(NRF_IRQ_TRIG)
			Task_NRF_TeleCom();
		else 
			led2 = 1;
		
		
		
		
		if(Trig_LED_Scan)
		{
			Trig_LED_Scan = 0;
			led3 = !led3;
		}
		else ;
		
		delay_ms(1);
	}
}	
