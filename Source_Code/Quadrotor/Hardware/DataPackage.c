/*
	TeleCtrl_RFbuf[20]
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-
|	0	|	1	|	2	|	3	|	4	|	5	|	6	|	7	|	8	|	9	|	10	|
+=======+=======+=======+=======+=======+=======+=======+=======+=======+=======+=======+=
| Token	| THR_H	| THR_L	| YAW_H | YAW_L | PIT_H | PIT_L | ROL_H | ROL_L | BAT_H | BAT_L |
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-

										+---------------+-------+-------+-------+-------+
										|		11		|	12	|	13	|	14	|	15	|
										+===============+=======+=======+=======+=======+
										|H4:断联 L4:解锁|
										+---------------|-------+-------+-------+-------+
*/
/*
	Aircraft_RFbuf[10]
+-------+---------------+---------------+---------------+---------------+---------------+-
|	0	|		1		|		2		|		3		|		4		|		5		|
+=======+===============+===============+===============+===============+===============+=
| Token	|  Euler_PIT_H	|  Euler_PIT_L	| Euler_ROLL_H	| Euler_ROLL_L	|  Euler_YAW_H	|
+-------+---------------+---------------+---------------+---------------+---------------+-

										+---------------+-------+-------+---------------+
										|		6		|	7	|	8	|		9		|
										+===============+=======+=======+===============+
										|  Euler_YAW_L	| BAT_H | BAT_L |H4:断联 L4:解锁|
										+---------------+-------+-------+---------------+
*/

#include "DataPackage.h"



extern u8	Aircraft_RFbuf[10];
extern u8	TeleCtrl_RFbuf[20];
extern u16	adc_battery;
extern float pitch, roll, yaw;

u16		rx_adc_thrust = 0;
short 	rx_adc_pitch = 0;
short 	rx_adc_roll = 0;
short 	rx_adc_yaw = 0;		//遥控器应该提前把ADC值换算成正负的角度值再传给飞机, 以便减轻飞机的负担
u8		THR_unlock = 0;




//数据打包
void Data_Package_to_TX()
{
	/* 对回传的数据进行打包 */
	Aircraft_RFbuf[0] = rx_adc_thrust / 20;			//注意uchar最大255

	Aircraft_RFbuf[1] = (short)(pitch*10) >> 8;
	Aircraft_RFbuf[2] = (short)(pitch*10) & 0x00FF;
	Aircraft_RFbuf[3] = (short)(roll*10) >> 8;
	Aircraft_RFbuf[4] = (short)(roll*10) & 0x00FF;
	Aircraft_RFbuf[5] = (short)(yaw*10) >> 8;
	Aircraft_RFbuf[6] = (short)(yaw*10) & 0x00FF;
	Aircraft_RFbuf[7] = adc_battery >> 8;
	Aircraft_RFbuf[8] = adc_battery & 0x00FF;
}





//数据拆包
void Data_UnPack_from_RX()
{
	/* 对接受的数据进行拆包 */
	rx_adc_thrust = (TeleCtrl_RFbuf[1]<<8) + TeleCtrl_RFbuf[2];
	rx_adc_pitch  = (short)( (TeleCtrl_RFbuf[5]<<8) + TeleCtrl_RFbuf[6] );	// +7/-7 degree
	rx_adc_roll   = (short)( (TeleCtrl_RFbuf[7]<<8) + TeleCtrl_RFbuf[8] );	// +7/-7 degree
	
	THR_unlock = TeleCtrl_RFbuf[11] & 0x0F;
}








