#include "led.h"
#include "key.h"
#include "delay.h"
#include "sys.h"
//#include "usart.h"
#include "oled.h"
#include "exti.h"
#include "dma.h"
#include "adc.h"
#include "si24r1.h"
#include "sdio_sdcard.h"
#include "extfunc.h"
#include "timer.h"
#include "IRQHandler.h"
//#include "i2c.h"
//#include <stdlib.h>//to use abs()

#if defined USB_MOUSE    //���붨��USB��깦��,��ǰ��IRQHandeler.h�޸�

	/* to use STM32-USB */
	#include "hw_config.h"
	#include "usb_lib.h"
	#include "usb_pwr.h"

#endif

/*extern*/u16 ADC_Mem[10];			//���߱���������������c�ļ��ж����,�ñ������Լ�ȥ��(��dma.c)
extern FRESULT	f_res;				//File function return code (FRESULT), ������extfunc.c
extern FATFS 	fs;					//FatFs�ļ�ϵͳ�ṹ��, ������extfunc.c
extern FIL		fp;					//�ļ��ṹ��ָ��, ������extfunc.c
extern UINT		bw,br;				//��ǰ�ܹ�д��/��ȡ���ֽ���(����f_read(),f_write())
extern FILINFO	fno;				//�ļ���Ϣ, ������extfunc.c
extern char		filename[20][127];	//���Ըĳ�malloc(), ������extfunc.c
extern char 	cur_path[10];		//���浱ǰ���ڵ�·��, ������extfunc.c
extern u32 	tot_size, fre_size, sd_size;	//FatFsϵͳSD��������, ������extfunc.c
//extern u8	fnum;	//·���µ��ļ�(��)����
//extern SD_CardInfo SDCardInfo;

#if defined USB_MOUSE
	extern u8 PrevXferComplete;					//�����ж�USBͨѶ�Ƿ����(��hw_config.c��line216��Ϊû����0, �������ö���1)
	u8 once = 1;								//USB�ӻ���ʼ������һ��
	u8 mousekey = 0x00;							//��ʼֵ(bit3��Ϊ1)
#endif

//��Ϊ���ֱ������0x80=128>ascii, ���Ա���Ϊstr[]Ӧ�ö���� u8* ��ʽ
/*char*/u8	str[256];		//�洢��SD����txt�ļ��������ַ���
//char	bin_pic[680];	//�洢��SD����bin�ļ�������һ֡ͼ��


u8	left_key_value = 0;
u8 	right_key_value= 0x00;//����ȡ��
u8	user1_key_value = 0;
u8	user2_key_value = 0;
u8	THR_unlock = 0x00;//Ĭ��0x00Ϊ����, 0000_0101Ϊ����, ����Ϊ�˰�ȫ�������0x05����һ�Ų��ý���.
u8	upper_item = 0;
u8	lower_item = 0;
u8	RF_detected = 0, SD_detected = 0, FAT_mounted = 0;//û�м�⵽SI24R1, û�в���SD��, û�й���FAT


u8	Aircraft_RFbuf[10];
u8	TeleCtrl_RFbuf[20];

u16 adc_thrust;	//�����Ƹ�
u16 adc_yaw;	//��תƫ����
u16 adc_pitch;	//ǰ������
u16 adc_roll;	//���Ҳ���
u16 adc_battery;//﮵�ص�ѹ, (2.7~4.2V)/2 = (1.35~2.1V)

u8	Trig_NRF_TX = 0;		// 20Hz
u8	Trig_LED_Scan = 0;		// 4Hz
u8	Trig_oled_FPS = 0;		// 25Hz
u8	Trig_adc_bat = 0;		// 4Hz


u8	surface, episode;//#surface=��ͼ��������, #episode=ĳ��surface�����Ľ���


int main(void)
{
//	u16 i=0, j=1;
	u8 k1=0, k2=0;
	
	short tx_pitch;
	short tx_roll;
	
	
	short rx_Euler_Pit;
	short rx_Euler_Rol;
	short rx_Euler_Yaw;
	u16 rx_adc_battery;
	
	
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����2(2bit��ռ��,2bit��Ӧ��)
	delay_init();
	//uart_init(115200);
	exti_init();//�˺����ڲ�������key_init()
	led_init();
	dma_config(DMA1_Channel1,(u32)&ADC1->DR,(u32)&ADC_Mem, 10);//��ʼ��ADC1��DMA1_CH1, DMA����10������
	adc1_init();//ADC��ʼ��
	adc_dma_start();
	
	TIM3_Config_Init(99, 719);//������Ƶ��Ϊ72/(719+1)=100KHz, ���Ƶ��=100K/(99+1)=1KHz, ��1ms���жϴ���

	si24r1_spi_init();

	oled_spi_init();
	
	//oled_ssd1306_init();
	oled_ssd1351_init();
	
//	oled_show_ColorImage(0,0,"delete",1);
//	oled_refresh();
//	while(right_key_value==0);

	//#if defined SSD1306_C128_R64
	//oled_draw_line(127,0,127,63,1);	//-------------��Ļ���ҵĴ��� ------------------
	//oled_draw_line(0,14,127,14,1);	//-------------��Ļ�Ϸ��ĺ��� ------------------
	//#endif
	//oled_refresh();

	
	TeleCtrl_RFbuf[0] = 77;
	
	//���SI24R1�Ƿ�װ����
	if(si24r1_detect())
	{
		RF_detected = 1;//��װ����
	}
	else
	{
		RF_detected = 0;
		oled_show_string(0, 1, (u8*)"SI24R1 not found.", 12, CYAN, BLACK);
		oled_refresh();
	}
	si24r1_TX_mode();

	while(1)//-------------------- while(1) ------------------------- while(1) -------------------------
	{
//		static u8 k=0;
		while(1)//------------------------------------------  while of surface_icon
		{
			//ÿ1Hz��ʾ������ˮƽ�ָ���
			oled_show_string(122, 1, (u8*)"%", 12, CYAN, BLACK);
#if defined SSD1306_C128_R64
			oled_draw_line(0, 14, 127, 14, 1);
#else
			oled_draw_line(0, 14, 127, 14, CYAN);
#endif
			
			if(Trig_adc_bat)		// 10Hz
			{
				Trig_adc_bat = 0;
				adc_battery += (ADC_Mem[4]+ADC_Mem[9])/2;	//����10��
				k1++;
				if(k1==10)
				{
					adc_battery /= 10;
					oled_show_num_hide0(104, 1,Lithium_Battery_percent((float)adc_battery*(3.3f/4095)*2), 3, 12, CYAN, BLACK);
					k1 = 0;
					adc_battery = 0;//�������0��ᱣ����һ�εľ�ֵ, �����ӵڶ��ο�ʼ�ͱ���ó���11��
				}
				else ;
			}
			else ;
			
			if(Trig_LED_Scan)		// 2Hz
			{
				Trig_LED_Scan = 0;
				led3 = !led3;
			}
			else ;
			
			if(user1_key_value)
			{
				user1_key_value = 0;
				surface++;
			}
			else ;
			if(surface >= 4)	surface = 0;	//surface����Ŀǰֻ��4��, ����ɾ��ʱ�ǵ��޸Ĵ����
			else ;
			switch(surface)
			{
				case 0:
					oled_show_Bin_Array(40, 15, icon_drone, 48, 48, 0x01, 6);
					//oled_show_Icon(40, 15, Icon_drone, 48, 1);	//surface_drone_AutoCtrl
					oled_refresh();
					break;
				case 1:
					oled_show_Bin_Array(40, 15, icon_sdcard, 48, 48, 0x01, 6);
					//oled_show_Icon(40, 15, Icon_sdcard, 48, 1);	//surface_sdcard_FAT32
					oled_refresh();
					break;
#if defined USB_MOUSE
				case 2:
					oled_show_Bin_Array(40, 15, icon_mouse, 48, 48, 0x01, 6);
					//oled_show_Icon(40, 15, Icon_mouse, 48, 1);	//surface_mouse_STM32-USB-slave
					oled_refresh();
					break;
				case 3:
#else
				case 2:
#endif
					oled_show_Bin_Array(40, 15, icon_remote, 48, 48, 0x01, 6);
					//oled_show_Icon(40, 15, Icon_remote, 48, 1);	//surface_remote_SelfParameter
					oled_refresh();
					break;
				default:
					break;
			}
			if(right_key_value)			//�������ȷ�������˳���ǰsurfaceѡ�����
			{
				right_key_value = 0;	//��תsurface����Ӧ����0���м�ֵ, ���ǽ���episode�Ƿ�����ֵ�������д��о�
				left_key_value = 0; user1_key_value = 0; user2_key_value = 0;
				adc_battery = 0;		//��֤��remoter�����µ�adc_bat��ʼֵ��0
				oled_clear();			//��תҳ��ʱ˳������
				break;
			}
			else ;
		}		
		
		
		while(surface==0)//------------------------------------------  surface_drone
		{
			if(left_key_value)			//������·��ؼ����˻���surfaceѡ�����(ʵ��Ӧ�ò����˵�ǰһ��episodeֱ��surface����)
			{
				left_key_value = 0;		//��תsurface����Ӧ����0���м�ֵ, ���ǽ���episode�Ƿ�����ֵ�������д��о�
				right_key_value = 0; user1_key_value = 0; user2_key_value = 0;
				THR_unlock = 0x00;		//Ĭ������
				adc_battery = 0;		//��֤���ص�icon����ʱadc_bat�ĳ�ʼֵ��0
				led2 = 1;				//�ر�ͨѶָʾ��
				led3 = 1;				//�رչ���ָʾ��
				oled_clear();			//��תҳ��ʱ˳������
				break;
			}
			else ;
			
			
			if(Trig_NRF_TX)		// 20Hz
			{
				Trig_NRF_TX = 0;
				if(si24r1_TX_packet(TeleCtrl_RFbuf, Aircraft_RFbuf) & TX_OK)//���ͳɹ�
					led2 = 0;
				else
					led2 = 1;
			}
			else ;
			
//			if(si24r1_RX_RSSI()==0)			//���RSSI�ź�ǿ�ȵ���-64dBm
//				led1 = 1;
//			else
//				led1 = 0;

			/*!< Check PA8 to detect SD */
//			if(GPIO_ReadInputDataBit(SD_DETECT_GPIO_PORT, SD_DETECT_PIN) != Bit_RESET)
//				led1 = 0;	//SD��û�в���, �����
//			else
//				led1 = 1;	//��⵽SD��, �����
			
			
			//oled_show_num_hide0(102, 0, Aircraft_RFbuf[0], 3, 12, 1);//��ʾ�������ش�������
			
			rx_Euler_Pit = (short)(Aircraft_RFbuf[1]<<8) + Aircraft_RFbuf[2];
			oled_show_float(102, 13,(float)(rx_Euler_Pit/10.0f), 2, 1, 12, CYAN, BLACK);

			rx_Euler_Rol = (short)(Aircraft_RFbuf[3]<<8) + Aircraft_RFbuf[4];
			oled_show_float(102, 26,(float)(rx_Euler_Rol/10.0f), 2, 1, 12, CYAN, BLACK);

			rx_Euler_Yaw = (short)(Aircraft_RFbuf[5]<<8) + Aircraft_RFbuf[6];
			oled_show_float(102, 39,(float)(rx_Euler_Yaw/10.0f), 2, 1, 12, CYAN, BLACK);

			//�ɻ��ĵ�ص���
			rx_adc_battery = (Aircraft_RFbuf[7]<<8) + Aircraft_RFbuf[8];
			oled_show_float(102, 0,(float)rx_adc_battery*(3.3f/4095)*2+0.005, 1, 2, 12, CYAN, BLACK);
			

			
			adc_thrust	= (ADC_Mem[0]+ADC_Mem[5])/2;
			adc_yaw		= (ADC_Mem[1]+ADC_Mem[6])/2;
			adc_pitch	= (ADC_Mem[2]+ADC_Mem[7])/2;
			adc_roll 	= (ADC_Mem[3]+ADC_Mem[8])/2;
			adc_battery = (ADC_Mem[4]+ADC_Mem[9])/2;

/*	TeleCtrl_RFbuf[20]
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+---------------+
|	0	|	1	|	2	|	3	|	4	|	5	|	6	|	7	|	8	|	9	|	10	|		11		|
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+---------------|
| Token	| THR_H	| THR_L	| YAW_H | YAW_L | PIT_H | PIT_L | ROL_H | ROL_L | BAT_H | BAT_L |H4:���� L4:����|
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+---------------|
*/
/*	Aircraft_RFbuf[10]
+-------+---------------+---------------+---------------+---------------+---------------+---------------+-------+-------+---------------+
|	0	|		1		|		2		|		3		|		4		|		5		|		6		|	7	|	8	|		9		|
+-------+---------------+---------------+---------------+---------------+---------------+---------------+-------+-------+---------------|
| Token	|  Euler_PIT_H	|  Euler_PIT_L	| Euler_ROLL_H	| Euler_ROLL_L	|  Euler_YAW_H	|  Euler_YAW_L	| BAT_H | BAT_L |H4:���� L4:����|
+-------+---------------+---------------+---------------+---------------+---------------+---------------+-------+-------+---------------|
*/
			tx_pitch = ((short)adc_pitch - 2048)/290;	// +7/-7 degree
			tx_roll  = ((short)adc_roll  - 2048)/290;	// +7/-7 degree


			TeleCtrl_RFbuf[ 1] = adc_thrust >> 8;		TeleCtrl_RFbuf[ 2] = adc_thrust & 0x00FF;
			TeleCtrl_RFbuf[ 3] = 0 /*adc_yaw>>8*/;		TeleCtrl_RFbuf[ 4] = 0 /*adc_yaw & 0x00FF*/;
			TeleCtrl_RFbuf[ 5] = tx_pitch >> 8;			TeleCtrl_RFbuf[ 6] = tx_pitch & 0x00FF;
			TeleCtrl_RFbuf[ 7] = (-tx_roll) >> 8;		TeleCtrl_RFbuf[ 8] = (-tx_roll) & 0x00FF;
			TeleCtrl_RFbuf[ 9] = adc_battery >> 8;		TeleCtrl_RFbuf[10] = adc_battery & 0x00FF;
			
			TeleCtrl_RFbuf[11] = 0xFF & THR_unlock;
			
			if( (adc_pitch>2500)&&(upper_item==0) )			upper_item = 1;//��ҡ��������
			else if( (adc_pitch<1100)&&(lower_item==0) )	lower_item = 1;//��ҡ��������
			else ;
			
			
			oled_show_string(0,  0, (u8*)"Thr:     = 0.00V", 12, CYAN, BLACK);	//x, y, *p, lib, mode
			oled_show_string(0, 13, (u8*)"Yaw:     = 0.00V", 12, CYAN, BLACK);
			oled_show_string(0, 26, (u8*)"Pit:     = 0.00V", 12, CYAN, BLACK);
			oled_show_string(0, 39, (u8*)"Rol:     = 0.00V", 12, CYAN, BLACK);
			oled_show_string(0, 52, (u8*)"bat:     = 0.00V", 12, CYAN, BLACK);
			
			
			//��ʾADC������ֵ
			oled_show_num_hide0(24,  0,adc_thrust, 4, 12, CYAN, BLACK);	//mode=1Ϊ������ʾ,0λ��ɫ
			oled_show_num_hide0(24, 13,adc_yaw,    4, 12, CYAN, BLACK);
			oled_show_num_hide0(24, 26,adc_pitch,  4, 12, CYAN, BLACK);
			oled_show_num_hide0(24, 39,adc_roll,   4, 12, CYAN, BLACK);
			oled_show_num_hide0(24, 52,adc_battery,4, 12, CYAN, BLACK);
			
			//��ʾ��ѹ		
			oled_show_float(66,  0,(float)adc_thrust*(3.3f/4095), 1, 2, 12, CYAN, BLACK);	// thrust		
			oled_show_float(66, 13,(float)adc_yaw*(3.3f/4095), 1, 2, 12, CYAN, BLACK);		// yaw		
			oled_show_float(66, 26,(float)adc_pitch*(3.3f/4095), 1, 2, 12, CYAN, BLACK);	// pitch		
			oled_show_float(66, 39,(float)adc_roll*(3.3f/4095), 1, 2, 12, CYAN, BLACK);		// roll		
			oled_show_float(66, 52,(float)adc_battery*(3.3f/4095)*2, 1, 2, 12, CYAN, BLACK);// battery


			/* �������ֻ������if��, �ֶ�����ֻ������else if��, ����һ�ű���, �Ȳ��ظ�����Ҳ���ظ����� */
			if(user2_key_value &&(adc_thrust<400)&& !THR_unlock)	//���źܵ�+��ֵ��λ+��ǰ���� = ����
			{
				THR_unlock = 0x05;//0000_0101
				user2_key_value = 0;
			}
			else if(user2_key_value &&(adc_thrust<400)&& THR_unlock)//���Ž�С+��ֵ��λ+��ǰû�� = ����
			{
				THR_unlock = 0x00;//0000_0000
				user2_key_value = 0;
			}
//			else if(user2_key_value &&(adc_thrust>500)&& THR_unlock)//���Žϴ�+��ֵ��λ+��ǰû�� = ����
//			{
//				THR_unlock = THR_unlock;
//				user2_key_value = 0;
//			}
			else if(user2_key_value)	//ע�Ᵽ�ּ�ֵһֱ��0
			{
				user2_key_value = 0;
			}
			else ;	//����������Լ�������������к���, �������
			
			if(THR_unlock == 0x05)						//��0x05�������
				oled_show_string(96,52, (u8*)"/ On.", 12, CYAN, BLACK);
			else										//����ֵ��������
				oled_show_string(96,52, (u8*)"/Off.", 12, CYAN, BLACK);


			if(Trig_oled_FPS)		// 25Hz
			{
				Trig_oled_FPS = 0;
				oled_refresh();							//������ʾ
			}
			else ;
			
			if(Trig_LED_Scan)		// 2Hz
			{
				Trig_LED_Scan = 0;
				led3 = !led3;
			}
			else ;
			
			delay_ms(1);
		}//while-loop of drone

		
		
		while(surface==1)//------------------------------------------  surface_sdcard
		{			
			adc_thrust	= (ADC_Mem[0]+ADC_Mem[5])/2;
			adc_yaw		= (ADC_Mem[1]+ADC_Mem[6])/2;
			adc_pitch	= (ADC_Mem[2]+ADC_Mem[7])/2;
			adc_roll 	= (ADC_Mem[3]+ADC_Mem[8])/2;
			adc_battery = (ADC_Mem[4]+ADC_Mem[9])/2;
			
			if( (adc_pitch>2460)&&(upper_item==0) )			upper_item = 1;//��ҡ��������
			else if( (adc_pitch<1100)&&(lower_item==0) )	lower_item = 1;//��ҡ��������
			else ;

			
			if(GPIO_ReadInputDataBit(SD_DETECT_GPIO_PORT, SD_DETECT_PIN) != Bit_RESET)
			{
				SD_detected = 0;	//SD��û�в���
				FAT_mounted = 0;	//ǿ�ư�SD�� ��Ĭ���Ѿ����FAT
				led1 = 1;			//���
			}
			else
			{
				SD_detected = 1;	//��⵽SD��
			}
			
			
			
			static u8 j0 = 1;	//��Ŀ��λֵ j0 ���������õ�����ֻ�ܳ�ʼ��һ��, �ʴ˴�static����!
			if(lower_item && (adc_pitch>2001)&&(adc_pitch<2099))//�������ѡ����ҡ�˻�����
			{
				lower_item = 0;
				j0++;
				if(j0>3)	j0=1;	//j0ֻ��ȡ 1 2 3
			}
			else if(upper_item && (adc_pitch>2001)&&(adc_pitch<2099))//�������ѡ����ҡ�˻�����
			{
				upper_item = 0;
				j0--;
				if(j0<1)	j0=3;	//��ֹj0=-1, ���趨jֻ��ȡ 1 2 3
			}
			else ;
			
//			oled_show_string(11,  18, (u8*)"1. Card Capacity",12,(j0==1)?0:1);	//��ѡ�е���Ŀ���з�ɫ��ʾ
//			oled_show_string(11,  33, (u8*)"2. Filename List",12,(j0==2)?0:1);
//			oled_show_string(11,  48, (u8*)"3. Eject & Return",12,(j0==3)?0:1);
			//��ѡ�е���Ŀ���з�ɫ��ʾ
			oled_show_string(11,  18, (u8*)"1. Card Capacity", 12, (j0==1)?BLACK:CYAN, (j0==1)?CYAN:BLACK);
			oled_show_string(11,  33, (u8*)"2. Filename List", 12, (j0==2)?BLACK:CYAN, (j0==2)?CYAN:BLACK);
			oled_show_string(11,  48, (u8*)"3. Eject & Return",12, (j0==3)?BLACK:CYAN, (j0==3)?CYAN:BLACK);

			
			if(Trig_oled_FPS)//25Hz
			{
				Trig_oled_FPS = 0;
				oled_refresh();
			}
			else;			
//			delay_ms(1);
			
			// ������һ��
			if(right_key_value)					//�������ȷ���������episode����
			{
				right_key_value = 0;			//��תsurface����Ӧ����0���м�ֵ, ���ǽ���episode�Ƿ�����ֵ�������д��о�
				left_key_value = 0;
				user1_key_value = 0;
				user2_key_value = 0;			//ע�����surfaceǰ������0��user2, ����������drone, ��ᵼ��������Ĭ������
				
				if(SD_detected == 0)			//SD��û�в���
				{
					episode = 0;
				}
				else 
				{
					episode = j0;							
					oled_clear();
					if(FAT_mounted==0)
					{
						f_res = f_mount(&fs,"0:",1);	//����SD����FatFs�ļ�ϵͳ
						if(f_res==FR_OK){	
							FAT_mounted = 1;			//ָʾ�������ع�FAT, ��ֹ��ι���
							led1 = 0;					//����
						}
						else{
							FAT_mounted = 0;
							led1 = 1;
						}
					}else 
						led1 = 0;
				}
				
				
				switch(episode)
				{
					case 0://-------------------- episode 0: ������ʾ����
						while((left_key_value==0)&&(right_key_value==0)&&(user1_key_value==0)&&(user2_key_value==0))//��������˳�
						{
							oled_buffer_clear(9, 15, 117, 54);
#if defined SSD1306_C128_R64
							oled_draw_rectangle(11, 17, 115, 52, 1);
#else
							oled_draw_rectangle(11, 17, 115, 52, CYAN);
#endif
							oled_show_string(13, 19, (u8*)"    ! ERROR !    ", 12, CYAN, BLACK);
							oled_show_string(13, 40, (u8*)"SDcard not found.", 12, CYAN, BLACK);
							
							if(Trig_oled_FPS)//25Hz
							{
								Trig_oled_FPS = 0;
								oled_refresh();
							}
							else;
						}
						left_key_value=0; right_key_value=0; user1_key_value=0; user2_key_value=0;
						oled_clear();
						break;
					case 1://--------------------------- episode 1: SD����������
						ef_display_SDcapcity("0:");
						while((left_key_value==0)&&(right_key_value==0))//���Ϸ���������˳�
						{
							if(Trig_oled_FPS)//25Hz
							{
								Trig_oled_FPS = 0;
								oled_refresh();
							}
							else;
						}
						left_key_value=0; right_key_value=0; user1_key_value=0; user2_key_value=0;
						oled_clear();
						break;
					case 2://--------------------------- episode 2: �ļ��б����
						ef_FileList_iteration(cur_path);	//��������
						oled_clear();
						break;
					case 3://--------------------------- episode 3: ���FAT���˳�
						if(FAT_mounted==1)
						{
							f_mount(NULL,"0:",1);		//���FAT
							FAT_mounted = 0;
							led1 = 1;
						}else ;
						left_key_value=1;	//��֤��Eject SD����ȷ�����Ժ�˳�㴥�� ���ؼ� �Ա��˻ص�surface_sd����
						right_key_value=0; user1_key_value=0; user2_key_value=0;
						//oled_clear();
						break;
					default:
						left_key_value=0; right_key_value=0; user1_key_value=0; user2_key_value=0;
						oled_clear();
						break;
				}			
			}else ;
			
			// ������һ��
			if(left_key_value)					//������·��ؼ����˻���surfaceѡ�����(ʵ��Ӧ�ò����˵�ǰһ��episodeֱ��surface����)
			{
				left_key_value = 0;				//��תsurface����Ӧ����0���м�ֵ, ���ǽ���episode�Ƿ�����ֵ�������д��о�
				right_key_value = 0;
				user1_key_value = 0;
				user2_key_value = 0;			//ע��user2��ֵ����0, ������´ν���surface_drone֮��Ĭ������
				adc_battery = 0;				//��֤���ص�icon����ʱadc_bat�ĳ�ʼֵ��0
				j0 = 1;							//�˾�ȡ����surface_sd����Ŀ��λֵ�ļ�����(ÿ�ν��붼��1)
				if(FAT_mounted==1)
				{
					f_mount(NULL,"0:",1);		//���FAT
					FAT_mounted = 0;
					led1 = 1;
				}else 
					led1 = 1;
				
				oled_clear();					//��תҳ��ʱ˳������
				break;
			}
			else ;
		}//while-loop of sdcard	
							
							
#if defined USB_MOUSE		
		while(surface==2)//------------------------------------------  surface_mouse
		{
			// KEY1:	����Ҽ�			KEY2:	����������/˫��
			// KEY3:	�˳��˹���			KEY4:	����Ҽ�
			// ��ҡ��:	����=������		��ҡ��: ����άƽ��
			
			if(user1_key_value)			//����������°������˻���surfaceѡ�����
			{
				user1_key_value=0;
				left_key_value = 0; right_key_value=0; user2_key_value=0;
				adc_battery = 0;		//��֤���ص�icon����ʱadc_bat�ĳ�ʼֵ��0
				led1 = 1;				//Ϩ��Ƶ�
				oled_clear();			//��תҳ��ʱ˳������
				break;
			}
			else ;
			
			if(once){
				// Init and Config USB
				delay_ms(100);
				USB_Port_Set(0);
				delay_ms(400);
				USB_Port_Set(1);
				delay_ms(200);
				USB_Interrupts_Config();
				Set_USBClock();
				USB_Init();
				//delay_ms(100);
				
				oled_show_string(1,   8, (u8*)" USB Virtual Mouse: ", 12, CYAN, BLACK);
				oled_show_string(1,  21, (u8*)"KEY1=aux    KEY2=aim", 12, CYAN, BLACK);
				oled_show_string(1,  34, (u8*)"KEY3=back   KEY4=mid", 12, CYAN, BLACK);
				oled_show_string(1,  47, (u8*)"| = scroll  + = move", 12, CYAN, BLACK);
				oled_refresh();
				
				Joystick_Send(0, 0, 0, 0);
				user1_key_value=0; left_key_value = 0; right_key_value=0; user2_key_value=0;
				
				once = 0;
			}
			else ;
			if(bDeviceState == CONFIGURED)
				led1 = 0;	//USB���ӳɹ�, �Ƶ���
			else
				led1 = 1;	//ʧ����Ƶ���
			
			
			adc_thrust	= (ADC_Mem[0]+ADC_Mem[5])/2;
			//adc_yaw		= (ADC_Mem[1]+ADC_Mem[6])/2;
			adc_pitch	= (ADC_Mem[2]+ADC_Mem[7])/2;
			adc_roll 	= (ADC_Mem[3]+ADC_Mem[8])/2;
			//adc_battery = (ADC_Mem[4]+ADC_Mem[9])/2;
//			oled_show_float(50,  0,(float)adc_roll*(3.3f/4095), 1, 2, 8, 1);		// thrust
			
			signed char at = 0;
			if(adc_thrust > 2648)			at = 1;		//mousekey |= 0x02;
			else if(adc_thrust < 1448)		at = -1;	//mousekey &= 0xFD;
			else							at = 0;
			//char ay = adc_yaw/128 - 16;
			char ap;
			char ar;
			if(adc_pitch/64 > 32)		ap = adc_pitch/64 - 32;
			else if(adc_pitch/64 < 31)	ap = adc_pitch/64 - 31;
			else						ap = 0;
			if(adc_roll/64 > 32)		ar = adc_roll/64 - 32;
			else if(adc_roll/64 < 31)	ar = adc_roll/64 - 31;
			else						ar = 0;
			
			//u8 mousekey = 0x00;							//����main.c�Ķ�������
			
			// ����Ҽ�(aux)->KEY1
			if(left_key_value){
				mousekey |= 0x02;
				
				if(mousekey&0x04)
					mousekey &= 0xFB;	//���Ҫ�н���м�ģʽ������
				else ;
				
				if(key1==0){			//�����סkey1����, ������while����if�ᵼ��ֻ��Ӧ�Ҽ������Թ����ƶ�
					Joystick_Send(mousekey, ar, -ap, at);
					delay_ms(5);
				}
				else{
					left_key_value = 0;
					mousekey &= 0xFD;
				}
				//left_key_value = 0;
			}
			else ;


			// ������(aim)->KEY2
			if(right_key_value){
				mousekey |= 0x01;
				
				if(mousekey&0x04)
					mousekey &= 0xFB;	//�Ҽ�Ҫ�н���м�ģʽ������
				else ;
				
				if(key2==0){			//�����סkey2����
					Joystick_Send(mousekey, ar, -ap, at);//aroll,apitch,athrust�������������ʵ��Ч���޸�
					delay_ms(5);
				}
				else{
					right_key_value = 0;
					mousekey &= 0xFE;
				}
				//right_key_value = 0;
			}
			else ;


			// ����м�(mid)->KEY4
			// ����м������Ҽ���ͬ, �м�������֧�ֳ���������ȷ�����ֽ׶�
			//	1.�����ɿ������м�ģʽ
			//	2.�ٰ����ɿ� �� �������Ҽ� �����˳��м�ģʽ
			if((user2_key_value) && (mousekey&0x04)==0){
				user2_key_value = 0;
				mousekey |= 0x04;
			}
			else if((user2_key_value) && (mousekey&0x04)){
				user2_key_value = 0;
				mousekey &= 0xFB;
			}
			else ;

			
			
			if((bDeviceState == CONFIGURED)&&(PrevXferComplete)){
				Joystick_Send(mousekey, ar, -ap, at);
				led3 = 0;
				delay_ms(5);
			}
			else
				led3 = 1;
			
		}//while-loop of USBmouse
#endif
		
#if defined USB_MOUSE		
		while(surface==3)//------------------------------------------  surface_hardware_info
#else
		while(surface==2)
#endif
		{
			if(left_key_value || right_key_value)	//������·��ؼ����˻���surfaceѡ�����
			{
				left_key_value = 0;		//��תsurface����Ӧ����0���м�ֵ, ���ǽ���episode�Ƿ�����ֵ�������д��о�
				right_key_value=0; user1_key_value=0; user2_key_value=0;
				adc_battery = 0;		//��֤���ص�icon����ʱadc_bat�ĳ�ʼֵ��0
				oled_clear();			//��תҳ��ʱ˳������
				break;
			}
			else ;
			oled_show_string(1,  16, (u8*)"Creator:  bzq.Plexer", 12, CYAN, BLACK);
			oled_show_string(1,  31, (u8*)"Version:  Remoter v2", 12, CYAN, BLACK);
			oled_show_string(1,  48, (u8*)"    by  STM32_f103rc", 12, CYAN, BLACK);
			
			//ÿ1Hz��ʾ������ˮƽ�ָ���
			oled_show_string(122, 1, (u8*)"%", 12, CYAN, BLACK);
#if defined SSD1306_C128_R64
			oled_draw_line(0, 14, 127, 14, 1);
#else
			oled_draw_line(0, 14, 127, 14, CYAN);
#endif
			
			if(Trig_adc_bat)		// 10Hz
			{
				Trig_adc_bat = 0;
				adc_battery += (ADC_Mem[4]+ADC_Mem[9])/2;	//����10��
				k2++;
				if(k2==10)
				{
					adc_battery /= 10;
					oled_show_num_hide0(104, 1,Lithium_Battery_percent((float)adc_battery*(3.3f/4095)*2), 3, 12, CYAN, BLACK);
					k2 = 0;
					adc_battery = 0;//�������0��ᱣ����һ�εľ�ֵ, �����ӵڶ��ο�ʼ�ͱ���ó���11��
				}
				else ;
			}
			else ;
			
			if(Trig_oled_FPS)		//25Hz
			{
				Trig_oled_FPS = 0;
				oled_refresh();
			}
			else;
			if(Trig_LED_Scan)		// 2Hz
			{
				Trig_LED_Scan = 0;
				led3 = !led3;
			}
			else ;
//			oled_refresh();			
//			delay_ms(1);
		}//while-loop of hardware info
	}//while(1)
	
	/* ȡ������SD����FatFs�ļ�ϵͳ */
	//f_mount(NULL,"0:",1);
}


