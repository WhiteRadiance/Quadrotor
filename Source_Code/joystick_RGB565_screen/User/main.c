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

#if defined USB_MOUSE    //若想定义USB鼠标功能,请前往IRQHandeler.h修改

	/* to use STM32-USB */
	#include "hw_config.h"
	#include "usb_lib.h"
	#include "usb_pwr.h"

#endif

/*extern*/u16 ADC_Mem[10];			//告诉编译器变量在其他c文件中定义过,让编译器自己去找(在dma.c)
extern FRESULT	f_res;				//File function return code (FRESULT), 定义于extfunc.c
extern FATFS 	fs;					//FatFs文件系统结构体, 定义于extfunc.c
extern FIL		fp;					//文件结构体指针, 定义于extfunc.c
extern UINT		bw,br;				//当前总共写入/读取的字节数(用于f_read(),f_write())
extern FILINFO	fno;				//文件信息, 定义于extfunc.c
extern char		filename[20][127];	//可以改成malloc(), 定义于extfunc.c
extern char 	cur_path[10];		//保存当前所在的路径, 定义于extfunc.c
extern u32 	tot_size, fre_size, sd_size;	//FatFs系统SD卡的容量, 定义于extfunc.c
//extern u8	fnum;	//路径下的文件(夹)个数
//extern SD_CardInfo SDCardInfo;

#if defined USB_MOUSE
	extern u8 PrevXferComplete;					//用于判断USB通讯是否就绪(在hw_config.c中line216因为没有置0, 所以永久都是1)
	u8 once = 1;								//USB从机初始化仅需一次
	u8 mousekey = 0x00;							//初始值(bit3恒为1)
#endif

//因为汉字编码大于0x80=128>ascii, 所以必须为str[]应该定义成 u8* 格式
/*char*/u8	str[256];		//存储从SD卡的txt文件读出的字符串
//char	bin_pic[680];	//存储从SD卡的bin文件读出的一帧图像


u8	left_key_value = 0;
u8 	right_key_value= 0x00;//用于取反
u8	user1_key_value = 0;
u8	user2_key_value = 0;
u8	THR_unlock = 0x00;//默认0x00为锁定, 0000_0101为解锁, 不过为了安全起见除了0x05以外一概不得解锁.
u8	upper_item = 0;
u8	lower_item = 0;
u8	RF_detected = 0, SD_detected = 0, FAT_mounted = 0;//没有检测到SI24R1, 没有插入SD卡, 没有挂载FAT


u8	Aircraft_RFbuf[10];
u8	TeleCtrl_RFbuf[20];

u16 adc_thrust;	//油门推杆
u16 adc_yaw;	//旋转偏航角
u16 adc_pitch;	//前进后退
u16 adc_roll;	//左右侧移
u16 adc_battery;//锂电池电压, (2.7~4.2V)/2 = (1.35~2.1V)

u8	Trig_NRF_TX = 0;		// 20Hz
u8	Trig_LED_Scan = 0;		// 4Hz
u8	Trig_oled_FPS = 0;		// 25Hz
u8	Trig_adc_bat = 0;		// 4Hz


u8	surface, episode;//#surface=大图标主界面, #episode=某个surface下属的界面


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
	
	
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组2(2bit抢占级,2bit响应级)
	delay_init();
	//uart_init(115200);
	exti_init();//此函数内部调用了key_init()
	led_init();
	dma_config(DMA1_Channel1,(u32)&ADC1->DR,(u32)&ADC_Mem, 10);//初始化ADC1的DMA1_CH1, DMA缓存10个数据
	adc1_init();//ADC初始化
	adc_dma_start();
	
	TIM3_Config_Init(99, 719);//计数器频率为72/(719+1)=100KHz, 溢出频率=100K/(99+1)=1KHz, 即1ms的中断触发

	si24r1_spi_init();

	oled_spi_init();
	
	//oled_ssd1306_init();
	oled_ssd1351_init();
	
//	oled_show_ColorImage(0,0,"delete",1);
//	oled_refresh();
//	while(right_key_value==0);

	//#if defined SSD1306_C128_R64
	//oled_draw_line(127,0,127,63,1);	//-------------屏幕最右的垂线 ------------------
	//oled_draw_line(0,14,127,14,1);	//-------------屏幕上方的横线 ------------------
	//#endif
	//oled_refresh();

	
	TeleCtrl_RFbuf[0] = 77;
	
	//检查SI24R1是否安装正常
	if(si24r1_detect())
	{
		RF_detected = 1;//安装正常
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
			//每1Hz显示电量和水平分割线
			oled_show_string(122, 1, (u8*)"%", 12, CYAN, BLACK);
#if defined SSD1306_C128_R64
			oled_draw_line(0, 14, 127, 14, 1);
#else
			oled_draw_line(0, 14, 127, 14, CYAN);
#endif
			
			if(Trig_adc_bat)		// 10Hz
			{
				Trig_adc_bat = 0;
				adc_battery += (ADC_Mem[4]+ADC_Mem[9])/2;	//叠加10次
				k1++;
				if(k1==10)
				{
					adc_battery /= 10;
					oled_show_num_hide0(104, 1,Lithium_Battery_percent((float)adc_battery*(3.3f/4095)*2), 3, 12, CYAN, BLACK);
					k1 = 0;
					adc_battery = 0;//如果不清0则会保留上一次的均值, 这样从第二次开始就必须得除以11了
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
			if(surface >= 4)	surface = 0;	//surface界面目前只有4个, 增补删减时记得修改此语句
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
			if(right_key_value)			//如果按下确定键则退出当前surface选择界面
			{
				right_key_value = 0;	//跳转surface界面应该清0所有键值, 但是进入episode是否保留键值记忆性有待研究
				left_key_value = 0; user1_key_value = 0; user2_key_value = 0;
				adc_battery = 0;		//保证在remoter界面下的adc_bat初始值是0
				oled_clear();			//跳转页面时顺便清屏
				break;
			}
			else ;
		}		
		
		
		while(surface==0)//------------------------------------------  surface_drone
		{
			if(left_key_value)			//如果按下返回键则退回至surface选择界面(实际应该不断退到前一个episode直至surface界面)
			{
				left_key_value = 0;		//跳转surface界面应该清0所有键值, 但是进入episode是否保留键值记忆性有待研究
				right_key_value = 0; user1_key_value = 0; user2_key_value = 0;
				THR_unlock = 0x00;		//默认上锁
				adc_battery = 0;		//保证跳回到icon界面时adc_bat的初始值是0
				led2 = 1;				//关闭通讯指示灯
				led3 = 1;				//关闭工作指示灯
				oled_clear();			//跳转页面时顺便清屏
				break;
			}
			else ;
			
			
			if(Trig_NRF_TX)		// 20Hz
			{
				Trig_NRF_TX = 0;
				if(si24r1_TX_packet(TeleCtrl_RFbuf, Aircraft_RFbuf) & TX_OK)//发送成功
					led2 = 0;
				else
					led2 = 1;
			}
			else ;
			
//			if(si24r1_RX_RSSI()==0)			//如果RSSI信号强度低于-64dBm
//				led1 = 1;
//			else
//				led1 = 0;

			/*!< Check PA8 to detect SD */
//			if(GPIO_ReadInputDataBit(SD_DETECT_GPIO_PORT, SD_DETECT_PIN) != Bit_RESET)
//				led1 = 0;	//SD卡没有插入, 则灯亮
//			else
//				led1 = 1;	//检测到SD卡, 则灯灭
			
			
			//oled_show_num_hide0(102, 0, Aircraft_RFbuf[0], 3, 12, 1);//显示飞行器回传的数据
			
			rx_Euler_Pit = (short)(Aircraft_RFbuf[1]<<8) + Aircraft_RFbuf[2];
			oled_show_float(102, 13,(float)(rx_Euler_Pit/10.0f), 2, 1, 12, CYAN, BLACK);

			rx_Euler_Rol = (short)(Aircraft_RFbuf[3]<<8) + Aircraft_RFbuf[4];
			oled_show_float(102, 26,(float)(rx_Euler_Rol/10.0f), 2, 1, 12, CYAN, BLACK);

			rx_Euler_Yaw = (short)(Aircraft_RFbuf[5]<<8) + Aircraft_RFbuf[6];
			oled_show_float(102, 39,(float)(rx_Euler_Yaw/10.0f), 2, 1, 12, CYAN, BLACK);

			//飞机的电池电量
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
| Token	| THR_H	| THR_L	| YAW_H | YAW_L | PIT_H | PIT_L | ROL_H | ROL_L | BAT_H | BAT_L |H4:断联 L4:解锁|
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+---------------|
*/
/*	Aircraft_RFbuf[10]
+-------+---------------+---------------+---------------+---------------+---------------+---------------+-------+-------+---------------+
|	0	|		1		|		2		|		3		|		4		|		5		|		6		|	7	|	8	|		9		|
+-------+---------------+---------------+---------------+---------------+---------------+---------------+-------+-------+---------------|
| Token	|  Euler_PIT_H	|  Euler_PIT_L	| Euler_ROLL_H	| Euler_ROLL_L	|  Euler_YAW_H	|  Euler_YAW_L	| BAT_H | BAT_L |H4:断联 L4:解锁|
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
			
			if( (adc_pitch>2500)&&(upper_item==0) )			upper_item = 1;//右摇杆向上推
			else if( (adc_pitch<1100)&&(lower_item==0) )	lower_item = 1;//右摇杆向下推
			else ;
			
			
			oled_show_string(0,  0, (u8*)"Thr:     = 0.00V", 12, CYAN, BLACK);	//x, y, *p, lib, mode
			oled_show_string(0, 13, (u8*)"Yaw:     = 0.00V", 12, CYAN, BLACK);
			oled_show_string(0, 26, (u8*)"Pit:     = 0.00V", 12, CYAN, BLACK);
			oled_show_string(0, 39, (u8*)"Rol:     = 0.00V", 12, CYAN, BLACK);
			oled_show_string(0, 52, (u8*)"bat:     = 0.00V", 12, CYAN, BLACK);
			
			
			//显示ADC的量化值
			oled_show_num_hide0(24,  0,adc_thrust, 4, 12, CYAN, BLACK);	//mode=1为正常显示,0位反色
			oled_show_num_hide0(24, 13,adc_yaw,    4, 12, CYAN, BLACK);
			oled_show_num_hide0(24, 26,adc_pitch,  4, 12, CYAN, BLACK);
			oled_show_num_hide0(24, 39,adc_roll,   4, 12, CYAN, BLACK);
			oled_show_num_hide0(24, 52,adc_battery,4, 12, CYAN, BLACK);
			
			//显示电压		
			oled_show_float(66,  0,(float)adc_thrust*(3.3f/4095), 1, 2, 12, CYAN, BLACK);	// thrust		
			oled_show_float(66, 13,(float)adc_yaw*(3.3f/4095), 1, 2, 12, CYAN, BLACK);		// yaw		
			oled_show_float(66, 26,(float)adc_pitch*(3.3f/4095), 1, 2, 12, CYAN, BLACK);	// pitch		
			oled_show_float(66, 39,(float)adc_roll*(3.3f/4095), 1, 2, 12, CYAN, BLACK);		// roll		
			oled_show_float(66, 52,(float)adc_battery*(3.3f/4095)*2, 1, 2, 12, CYAN, BLACK);// battery


			/* 解除锁定只发生在if下, 手动上锁只发生在else if下, 其余一概保持, 既不重复上锁也不重复解锁 */
			if(user2_key_value &&(adc_thrust<400)&& !THR_unlock)	//油门很低+键值置位+当前被锁 = 解锁
			{
				THR_unlock = 0x05;//0000_0101
				user2_key_value = 0;
			}
			else if(user2_key_value &&(adc_thrust<400)&& THR_unlock)//油门较小+键值置位+当前没锁 = 上锁
			{
				THR_unlock = 0x00;//0000_0000
				user2_key_value = 0;
			}
//			else if(user2_key_value &&(adc_thrust>500)&& THR_unlock)//油门较大+键值置位+当前没锁 = 忽视
//			{
//				THR_unlock = THR_unlock;
//				user2_key_value = 0;
//			}
			else if(user2_key_value)	//注意保持键值一直是0
			{
				user2_key_value = 0;
			}
			else ;	//除以上情况对加锁解锁请求进行忽视, 不予理睬
			
			if(THR_unlock == 0x05)						//仅0x05代表解锁
				oled_show_string(96,52, (u8*)"/ On.", 12, CYAN, BLACK);
			else										//其余值都是锁定
				oled_show_string(96,52, (u8*)"/Off.", 12, CYAN, BLACK);


			if(Trig_oled_FPS)		// 25Hz
			{
				Trig_oled_FPS = 0;
				oled_refresh();							//更新显示
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
			
			if( (adc_pitch>2460)&&(upper_item==0) )			upper_item = 1;//右摇杆向上推
			else if( (adc_pitch<1100)&&(lower_item==0) )	lower_item = 1;//右摇杆向下推
			else ;

			
			if(GPIO_ReadInputDataBit(SD_DETECT_GPIO_PORT, SD_DETECT_PIN) != Bit_RESET)
			{
				SD_detected = 0;	//SD卡没有插入
				FAT_mounted = 0;	//强制拔SD卡 则默认已经解除FAT
				led1 = 1;			//灭灯
			}
			else
			{
				SD_detected = 1;	//检测到SD卡
			}
			
			
			
			static u8 j0 = 1;	//条目定位值 j0 仅在下面用到但是只能初始化一次, 故此处static声明!
			if(lower_item && (adc_pitch>2001)&&(adc_pitch<2099))//如果向下选择且摇杆回中了
			{
				lower_item = 0;
				j0++;
				if(j0>3)	j0=1;	//j0只能取 1 2 3
			}
			else if(upper_item && (adc_pitch>2001)&&(adc_pitch<2099))//如果向上选择且摇杆回中了
			{
				upper_item = 0;
				j0--;
				if(j0<1)	j0=3;	//防止j0=-1, 故设定j只能取 1 2 3
			}
			else ;
			
//			oled_show_string(11,  18, (u8*)"1. Card Capacity",12,(j0==1)?0:1);	//对选中的条目进行反色显示
//			oled_show_string(11,  33, (u8*)"2. Filename List",12,(j0==2)?0:1);
//			oled_show_string(11,  48, (u8*)"3. Eject & Return",12,(j0==3)?0:1);
			//对选中的条目进行反色显示
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
			
			// 进入下一级
			if(right_key_value)					//如果按下确定键则进入episode界面
			{
				right_key_value = 0;			//跳转surface界面应该清0所有键值, 但是进入episode是否保留键值记忆性有待研究
				left_key_value = 0;
				user1_key_value = 0;
				user2_key_value = 0;			//注意进入surface前这里清0了user2, 如果进入的是drone, 则会导致四旋翼默认上锁
				
				if(SD_detected == 0)			//SD卡没有插入
				{
					episode = 0;
				}
				else 
				{
					episode = j0;							
					oled_clear();
					if(FAT_mounted==0)
					{
						f_res = f_mount(&fs,"0:",1);	//挂载SD卡的FatFs文件系统
						if(f_res==FR_OK){	
							FAT_mounted = 1;			//指示曾经挂载过FAT, 防止多次挂载
							led1 = 0;					//亮灯
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
					case 0://-------------------- episode 0: 错误提示界面
						while((left_key_value==0)&&(right_key_value==0)&&(user1_key_value==0)&&(user2_key_value==0))//按任意键退出
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
					case 1://--------------------------- episode 1: SD卡容量界面
						ef_display_SDcapcity("0:");
						while((left_key_value==0)&&(right_key_value==0))//按上方的任意键退出
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
					case 2://--------------------------- episode 2: 文件列表界面
						ef_FileList_iteration(cur_path);	//迭代函数
						oled_clear();
						break;
					case 3://--------------------------- episode 3: 解除FAT并退出
						if(FAT_mounted==1)
						{
							f_mount(NULL,"0:",1);		//解除FAT
							FAT_mounted = 0;
							led1 = 1;
						}else ;
						left_key_value=1;	//保证对Eject SD按下确定键以后顺便触发 返回键 以便退回到surface_sd界面
						right_key_value=0; user1_key_value=0; user2_key_value=0;
						//oled_clear();
						break;
					default:
						left_key_value=0; right_key_value=0; user1_key_value=0; user2_key_value=0;
						oled_clear();
						break;
				}			
			}else ;
			
			// 返回上一级
			if(left_key_value)					//如果按下返回键则退回至surface选择界面(实际应该不断退到前一个episode直至surface界面)
			{
				left_key_value = 0;				//跳转surface界面应该清0所有键值, 但是进入episode是否保留键值记忆性有待研究
				right_key_value = 0;
				user1_key_value = 0;
				user2_key_value = 0;			//注意user2键值被清0, 则会在下次进入surface_drone之后默认上锁
				adc_battery = 0;				//保证跳回到icon界面时adc_bat的初始值是0
				j0 = 1;							//此句取消了surface_sd下条目定位值的记忆性(每次进入都是1)
				if(FAT_mounted==1)
				{
					f_mount(NULL,"0:",1);		//解除FAT
					FAT_mounted = 0;
					led1 = 1;
				}else 
					led1 = 1;
				
				oled_clear();					//跳转页面时顺便清屏
				break;
			}
			else ;
		}//while-loop of sdcard	
							
							
#if defined USB_MOUSE		
		while(surface==2)//------------------------------------------  surface_mouse
		{
			// KEY1:	鼠标右键			KEY2:	鼠标左键单击/双击
			// KEY3:	退出此功能			KEY4:	鼠标右键
			// 左摇杆:	油门=鼠标滚轮		右摇杆: 鼠标二维平移
			
			if(user1_key_value)			//如果按下左下按键则退回至surface选择界面
			{
				user1_key_value=0;
				left_key_value = 0; right_key_value=0; user2_key_value=0;
				adc_battery = 0;		//保证跳回到icon界面时adc_bat的初始值是0
				led1 = 1;				//熄灭黄灯
				oled_clear();			//跳转页面时顺便清屏
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
				led1 = 0;	//USB连接成功, 黄灯亮
			else
				led1 = 1;	//失败则黄灯灭
			
			
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
			
			//u8 mousekey = 0x00;							//移至main.c的顶部定义
			
			// 鼠标右键(aux)->KEY1
			if(left_key_value){
				mousekey |= 0x02;
				
				if(mousekey&0x04)
					mousekey &= 0xFB;	//左键要有解除中键模式的能力
				else ;
				
				if(key1==0){			//如果按住key1不放, 这里用while不用if会导致只响应右键而忽略光标的移动
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


			// 鼠标左键(aim)->KEY2
			if(right_key_value){
				mousekey |= 0x01;
				
				if(mousekey&0x04)
					mousekey &= 0xFB;	//右键要有解除中键模式的能力
				else ;
				
				if(key2==0){			//如果按住key2不放
					Joystick_Send(mousekey, ar, -ap, at);//aroll,apitch,athrust的正负根据鼠标实际效果修改
					delay_ms(5);
				}
				else{
					right_key_value = 0;
					mousekey &= 0xFE;
				}
				//right_key_value = 0;
			}
			else ;


			// 鼠标中键(mid)->KEY4
			// 鼠标中键与左右键不同, 中键几乎不支持长按且有明确的两种阶段
			//	1.按下松开进入中键模式
			//	2.再按下松开 或 触碰左右键 即可退出中键模式
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
			if(left_key_value || right_key_value)	//如果按下返回键则退回至surface选择界面
			{
				left_key_value = 0;		//跳转surface界面应该清0所有键值, 但是进入episode是否保留键值记忆性有待研究
				right_key_value=0; user1_key_value=0; user2_key_value=0;
				adc_battery = 0;		//保证跳回到icon界面时adc_bat的初始值是0
				oled_clear();			//跳转页面时顺便清屏
				break;
			}
			else ;
			oled_show_string(1,  16, (u8*)"Creator:  bzq.Plexer", 12, CYAN, BLACK);
			oled_show_string(1,  31, (u8*)"Version:  Remoter v2", 12, CYAN, BLACK);
			oled_show_string(1,  48, (u8*)"    by  STM32_f103rc", 12, CYAN, BLACK);
			
			//每1Hz显示电量和水平分割线
			oled_show_string(122, 1, (u8*)"%", 12, CYAN, BLACK);
#if defined SSD1306_C128_R64
			oled_draw_line(0, 14, 127, 14, 1);
#else
			oled_draw_line(0, 14, 127, 14, CYAN);
#endif
			
			if(Trig_adc_bat)		// 10Hz
			{
				Trig_adc_bat = 0;
				adc_battery += (ADC_Mem[4]+ADC_Mem[9])/2;	//叠加10次
				k2++;
				if(k2==10)
				{
					adc_battery /= 10;
					oled_show_num_hide0(104, 1,Lithium_Battery_percent((float)adc_battery*(3.3f/4095)*2), 3, 12, CYAN, BLACK);
					k2 = 0;
					adc_battery = 0;//如果不清0则会保留上一次的均值, 这样从第二次开始就必须得除以11了
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
	
	/* 取消挂载SD卡的FatFs文件系统 */
	//f_mount(NULL,"0:",1);
}


