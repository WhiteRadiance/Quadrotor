#include "mpu_dmp_api.h"
/*
	USART_Config();
	
	//各中断优先级配置
	NVIC_PriorityConfig();
	
	//初始化i2c, 并将systick或delay初始化
	I2C_Config();
	
	//初始化MPU
	mpu_dmp_init();
	
	//设置哪些数据上报给上位机
	SetReportFlag(SET,SET);
	
	//接收DMP中断完成信号的EXTI初始化
	EXTI_Config();
	
	while(1){
		//开始循环读取数据, 并上报数据给上位机
		dmp_getdata();
	}
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//#include "USB_eMPL/descriptors.h"
//#include "USB_API/USB_Common/device.h"
//#include "USB_API/USB_Common/types.h"
//#include "USB_API/USB_Common/usb.h"
//#include "F5xx_F6xx_Core_Lib/HAL_UCS.h"
//#include "F5xx_F6xx_Core_Lib/HAL_PMM.h"
//#include "F5xx_F6xx_Core_Lib/HAL_FLASH.h"
//#include "USB_API/USB_CDC_API/UsbCdc.h"
//#include "usbConstructs.h"

//#include "msp430.h"
//#include "msp430_clock.h"
//#include "msp430_i2c.h"
//#include "msp430_interrupt.h"
#include "stm32f10x.h"
#include "i2c.h"

#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"

/* Data requested by client. */
#define PRINT_ACCEL     (0x01)			//向上位机上报加速计数据的标志
#define PRINT_GYRO      (0x02)			//向上位机上报陀螺仪数据的标志
#define PRINT_QUAT      (0x04)			//向上位机上报四元数数据的标志

#define ACCEL_ON        (0x01)			//指示加速计已经打开的标志
#define GYRO_ON         (0x02)			//指示陀螺仪已经打开的标志

#define MOTION          (0)				//指示MPU正在运动状态
#define NO_MOTION       (1)				//指示MPU处于静止状态

/* Starting sampling rate. */
#define DEFAULT_MPU_HZ  (100)

#define FLASH_SIZE      (512)
#define FLASH_MEM_START ((void*)0x1800)

struct rx_s {
    unsigned char header[3];
    unsigned char cmd;
};
struct hal_s {
    unsigned char sensors;				//用于指示用到了哪些传感器
    unsigned char dmp_on;				//用于指示开启/关闭DMP
    unsigned char wait_for_tap;			//等待手势/敲击操作
    volatile unsigned char new_gyro;	//指示是否有新数据已经准备好了
    unsigned short report;				//用于指示是否上报数据给上位机
    unsigned short dmp_features;		//用于指示开启了DMP的哪些功能
    unsigned char motion_int_mode;		//用于指示运动中断是否打开
    struct rx_s rx;						//从外部接收到的数据
};
static struct hal_s hal = {0};			//初始化hal, 全部成员都复位为0

/* USB RX binary semaphore. Actually, it's just a flag. Not included in struct
 * because it's declared extern elsewhere.
 */
volatile unsigned char rx_new;			//用于指示有没有接收到外部新传来的数据

/* 方向矩阵
 * The sensors can be mounted onto the board in any orientation. The mounting
 * matrix seen below tells the MPL how to rotate the raw data from thei
 * driver(s).
 * TODO: The following matrices refer to the configuration on an internal test
 * board at Invensense. If needed, please modify the matrices to match the
 * chip-to-body matrix for your particular set up.
 */
static signed char gyro_orientation[9] = {-1, 0, 0,
                                           0,-1, 0,
                                           0, 0, 1};

//enum packet_type_e {
//    PACKET_TYPE_ACCEL,
//    PACKET_TYPE_GYRO,
//    PACKET_TYPE_QUAT,
//    PACKET_TYPE_TAP,
//    PACKET_TYPE_ANDROID_ORIENT,
//    PACKET_TYPE_PEDO,
//    PACKET_TYPE_MISC
//};

/* Send data to the Python client application.
 * Data is formatted as follows:
 * packet[0]    = $
 * packet[1]    = packet type (see packet_type_e)
 * packet[2+]   = data
 */
//void send_packet(char packet_type, void *data)
//{
//#define MAX_BUF_LENGTH  (18)
//    char buf[MAX_BUF_LENGTH], length;

//    memset(buf, 0, MAX_BUF_LENGTH);
//    buf[0] = '$';
//    buf[1] = packet_type;

//    if (packet_type == PACKET_TYPE_ACCEL || packet_type == PACKET_TYPE_GYRO) {
//        short *sdata = (short*)data;
//        buf[2] = (char)(sdata[0] >> 8);
//        buf[3] = (char)sdata[0];
//        buf[4] = (char)(sdata[1] >> 8);
//        buf[5] = (char)sdata[1];
//        buf[6] = (char)(sdata[2] >> 8);
//        buf[7] = (char)sdata[2];
//        length = 8;
//    } else if (packet_type == PACKET_TYPE_QUAT) {
//        long *ldata = (long*)data;
//        buf[2] = (char)(ldata[0] >> 24);
//        buf[3] = (char)(ldata[0] >> 16);
//        buf[4] = (char)(ldata[0] >> 8);
//        buf[5] = (char)ldata[0];
//        buf[6] = (char)(ldata[1] >> 24);
//        buf[7] = (char)(ldata[1] >> 16);
//        buf[8] = (char)(ldata[1] >> 8);
//        buf[9] = (char)ldata[1];
//        buf[10] = (char)(ldata[2] >> 24);
//        buf[11] = (char)(ldata[2] >> 16);
//        buf[12] = (char)(ldata[2] >> 8);
//        buf[13] = (char)ldata[2];
//        buf[14] = (char)(ldata[3] >> 24);
//        buf[15] = (char)(ldata[3] >> 16);
//        buf[16] = (char)(ldata[3] >> 8);
//        buf[17] = (char)ldata[3];
//        length = 18;
//    } else if (packet_type == PACKET_TYPE_TAP) {
//        buf[2] = ((char*)data)[0];
//        buf[3] = ((char*)data)[1];
//        length = 4;
//    } else if (packet_type == PACKET_TYPE_ANDROID_ORIENT) {
//        buf[2] = ((char*)data)[0];
//        length = 3;
//    } else if (packet_type == PACKET_TYPE_PEDO) {
//        long *ldata = (long*)data;
//        buf[2] = (char)(ldata[0] >> 24);
//        buf[3] = (char)(ldata[0] >> 16);
//        buf[4] = (char)(ldata[0] >> 8);
//        buf[5] = (char)ldata[0];
//        buf[6] = (char)(ldata[1] >> 24);
//        buf[7] = (char)(ldata[1] >> 16);
//        buf[8] = (char)(ldata[1] >> 8);
//        buf[9] = (char)ldata[1];
//        length = 10;
//    } else if (packet_type == PACKET_TYPE_MISC) {
//        buf[2] = ((char*)data)[0];
//        buf[3] = ((char*)data)[1];
//        buf[4] = ((char*)data)[2];
//        buf[5] = ((char*)data)[3];
//        length = 6;
//    }
//    cdcSendDataWaitTilDone((BYTE*)buf, length, CDC0_INTFNUM, 100);
//}



/* These next two functions converts the orientation matrix (see
 * gyro_orientation) to a scalar representation for use by the DMP.
 * NOTE: These functions are borrowed from Invensense's MPL.
 */
unsigned short inv_row_2_scale(const signed char *row)
{
    unsigned short b;

    if (row[0] > 0)
        b = 0;
    else if (row[0] < 0)
        b = 4;
    else if (row[1] > 0)
        b = 1;
    else if (row[1] < 0)
        b = 5;
    else if (row[2] > 0)
        b = 2;
    else if (row[2] < 0)
        b = 6;
    else
        b = 7;      // error
    return b;
}

static inline unsigned short inv_orientation_matrix_to_scalar(
    const signed char *mtx)
{
    unsigned short scalar;

    /*
       XYZ  010_001_000 Identity Matrix
       XZY  001_010_000
       YXZ  010_000_001
       YZX  000_010_001
       ZXY  001_000_010
       ZYX  000_001_010
     */

    scalar = inv_row_2_scale(mtx);
    scalar |= inv_row_2_scale(mtx + 3) << 3;
    scalar |= inv_row_2_scale(mtx + 6) << 6;


    return scalar;
}

/* Handle sensor on/off combinations. */
//static void setup_gyro(void)
//{
//    unsigned char mask = 0;
//    if (hal.sensors & ACCEL_ON)
//        mask |= INV_XYZ_ACCEL;
//    if (hal.sensors & GYRO_ON)
//        mask |= INV_XYZ_GYRO;
//    /* If you need a power transition, this function should be called with a
//     * mask of the sensors still enabled. The driver turns off any sensors
//     * excluded from this mask.
//     */
//    mpu_set_sensors(mask);
//    if (!hal.dmp_on)
//        mpu_configure_fifo(mask);
//}

//手势回调函数
//static void tap_cb(unsigned char direction, unsigned char count)
//{
//    char data[2];
//    data[0] = (char)direction;
//    data[1] = (char)count;
//    send_packet(PACKET_TYPE_TAP, data);
//}

//安卓系统的方向回调函数
//static void android_orient_cb(unsigned char orientation)
//{
//    send_packet(PACKET_TYPE_ANDROID_ORIENT, &orientation);
//}

//重启msp430
//static inline void msp430_reset(void)
//{
//    PMMCTL0 |= PMMSWPOR;
//}

//重启stm32
void stm32_reset(void)
{
	__set_FAULTMASK(1);	// 0:打开所有中断     1:关闭所有中断
	NVIC_SystemReset();	// 系统复位
}

//自检 并 校正偏差
void run_self_test(void)
{
    int result;
//    char test_packet[4] = {0};
    long gyro[3], accel[3];

    result = mpu_run_self_test(gyro, accel);
    //result=7代表MPU6500, =3代表MPU6050
	if (result == 0x3) {
        /* Test passed. We can trust the gyro data here, so let's push it down
         * to the DMP.
         */
        float sens;
        unsigned short accel_sens;
        mpu_get_gyro_sens(&sens);
        gyro[0] = (long)(gyro[0] * sens);
        gyro[1] = (long)(gyro[1] * sens);
        gyro[2] = (long)(gyro[2] * sens);
        dmp_set_gyro_bias(gyro);
        mpu_get_accel_sens(&accel_sens);
		
		//accel_sens = 0;//加入此句话可以允许飞机斜着放进行校准
		
        accel[0] *= accel_sens;
        accel[1] *= accel_sens;
        accel[2] *= accel_sens;
        dmp_set_accel_bias(accel);
    }

    /* Report results. */
//    test_packet[0] = 't';
//    test_packet[1] = result;
//    send_packet(PACKET_TYPE_MISC, test_packet);
}

//Invensense官方上位机对飞机进行校准调试
//static void handle_input(void)
//{
//    char c;
//    const unsigned char header[3] = "inv";
//    unsigned long pedo_packet[2];

//    /* Read incoming byte and check for header.
//     * Technically, the MSP430 USB stack can handle more than one byte at a
//     * time. This example allows for easily switching to UART if porting to a
//     * different microcontroller.
//     */
//    rx_new = 0;
//    cdcReceiveDataInBuffer((BYTE*)&c, 1, CDC0_INTFNUM);
//    if (hal.rx.header[0] == header[0]) {
//        if (hal.rx.header[1] == header[1]) {
//            if (hal.rx.header[2] == header[2]) {
//                memset(&hal.rx.header, 0, sizeof(hal.rx.header));
//                hal.rx.cmd = c;
//            } else if (c == header[2])
//                hal.rx.header[2] = c;
//            else
//                memset(&hal.rx.header, 0, sizeof(hal.rx.header));
//        } else if (c == header[1])
//            hal.rx.header[1] = c;
//        else
//            memset(&hal.rx.header, 0, sizeof(hal.rx.header));
//    } else if (c == header[0])
//        hal.rx.header[0] = header[0];
//    if (!hal.rx.cmd)
//        return;

//    switch (hal.rx.cmd) {
//    /* These commands turn the hardware sensors on/off. */
//    case '8':
//        if (!hal.dmp_on) {
//            /* Accel and gyro need to be on for the DMP features to work
//             * properly.
//             */
//            hal.sensors ^= ACCEL_ON;
//            setup_gyro();
//        }
//        break;
//    case '9':
//        if (!hal.dmp_on) {
//            hal.sensors ^= GYRO_ON;
//            setup_gyro();
//        }
//        break;
//    /* The commands start/stop sending data to the client. */
//    case 'a':
//        hal.report ^= PRINT_ACCEL;
//        break;
//    case 'g':
//        hal.report ^= PRINT_GYRO;
//        break;
//    case 'q':
//        hal.report ^= PRINT_QUAT;
//        break;
//    /* The hardware self test can be run without any interaction with the
//     * MPL since it's completely localized in the gyro driver. Logging is
//     * assumed to be enabled; otherwise, a couple LEDs could probably be used
//     * here to display the test results.
//     */
//    case 't':
//        run_self_test();
//        break;
//    /* Depending on your application, sensor data may be needed at a faster or
//     * slower rate. These commands can speed up or slow down the rate at which
//     * the sensor data is pushed to the MPL.
//     *
//     * In this example, the compass rate is never changed.
//     */
//    case '1':
//        if (hal.dmp_on)
//            dmp_set_fifo_rate(10);
//        else
//            mpu_set_sample_rate(10);
//        break;
//    case '2':
//        if (hal.dmp_on)
//            dmp_set_fifo_rate(20);
//        else
//            mpu_set_sample_rate(20);
//        break;
//    case '3':
//        if (hal.dmp_on)
//            dmp_set_fifo_rate(40);
//        else
//            mpu_set_sample_rate(40);
//        break;
//    case '4':
//        if (hal.dmp_on)
//            dmp_set_fifo_rate(50);
//        else
//            mpu_set_sample_rate(50);
//        break;
//    case '5':
//        if (hal.dmp_on)
//            dmp_set_fifo_rate(100);
//        else
//            mpu_set_sample_rate(100);
//        break;
//    case '6':
//        if (hal.dmp_on)
//            dmp_set_fifo_rate(200);
//        else
//            mpu_set_sample_rate(200);
//        break;
//	case ',':
//        /* Set hardware to interrupt on gesture event only. This feature is
//         * useful for keeping the MCU asleep until the DMP detects as a tap or
//         * orientation event.
//         */
//        dmp_set_interrupt_mode(DMP_INT_GESTURE);
//        break;
//    case '.':
//        /* Set hardware to interrupt periodically. */
//        dmp_set_interrupt_mode(DMP_INT_CONTINUOUS);
//        break;
//    case '7':
//        /* Reset pedometer. */
//        dmp_set_pedometer_step_count(0);
//        dmp_set_pedometer_walk_time(0);
//        break;
//    case 'f':
//        /* Toggle DMP. */
//        if (hal.dmp_on) {
//            unsigned short dmp_rate;
//            hal.dmp_on = 0;
//            mpu_set_dmp_state(0);
//            /* Restore FIFO settings. */
//            mpu_configure_fifo(INV_XYZ_ACCEL | INV_XYZ_GYRO);
//            /* When the DMP is used, the hardware sampling rate is fixed at
//             * 200Hz, and the DMP is configured to downsample the FIFO output
//             * using the function dmp_set_fifo_rate. However, when the DMP is
//             * turned off, the sampling rate remains at 200Hz. This could be
//             * handled in inv_mpu.c, but it would need to know that
//             * inv_mpu_dmp_motion_driver.c exists. To avoid this, we'll just
//             * put the extra logic in the application layer.
//             */
//            dmp_get_fifo_rate(&dmp_rate);
//            mpu_set_sample_rate(dmp_rate);
//        } else {
//            unsigned short sample_rate;
//            hal.dmp_on = 1;
//            /* Both gyro and accel must be on. */
//            hal.sensors |= ACCEL_ON | GYRO_ON;
//            mpu_set_sensors(INV_XYZ_ACCEL | INV_XYZ_GYRO);
//            mpu_configure_fifo(INV_XYZ_ACCEL | INV_XYZ_GYRO);
//            /* Preserve current FIFO rate. */
//            mpu_get_sample_rate(&sample_rate);
//            dmp_set_fifo_rate(sample_rate);
//            mpu_set_dmp_state(1);
//        }
//        break;
//    case 'm':
//        /* Test the motion interrupt hardware feature. */
//        hal.motion_int_mode = 1;
//        break;
//    case 'p':
//        /* Read current pedometer count. */
//        dmp_get_pedometer_step_count(pedo_packet);
//        dmp_get_pedometer_walk_time(pedo_packet + 1);
//        send_packet(PACKET_TYPE_PEDO, pedo_packet);
//        break;
//    case 'x':
//        msp430_reset();
//        break;
//    case 'v':
//        /* Toggle LP quaternion.
//         * The DMP features can be enabled/disabled at runtime. Use this same
//         * approach for other features.
//         */
//        hal.dmp_features ^= DMP_FEATURE_6X_LP_QUAT;
//        dmp_enable_feature(hal.dmp_features);
//        break;
//    default:
//        break;
//    }
//    hal.rx.cmd = 0;
//}



//陀螺仪数据已准备好
/* Every time new gyro data is available, this function is called in an
 * ISR context. In this example, it sets a flag protecting the FIFO read
 * function.
 */
void gyro_data_ready_cb(void)
{
    hal.new_gyro = 1;
}



/* Set up MSP430 peripherals. */
//static inline void platform_init(void)
//{
//	WDTCTL = WDTPW | WDTHOLD;
//    SetVCore(2);
//    msp430_clock_init(12000000L, 2);
//    if (USB_init() != kUSB_succeed)
//        msp430_reset();
//    msp430_i2c_enable();
//    msp430_int_init();

//    USB_setEnabledEvents(kUSB_allUsbEvents);
//    if (USB_connectionInfo() & kUSB_vbusPresent){
//        if (USB_enable() == kUSB_succeed){
//            USB_reset();
//            USB_connect();
//        } else
//            msp430_reset();
//    }
//}




//初始化MPU6050, 并自检和校准
u8 mpu_dmp_init(void)
{
	int result;
//    unsigned char accel_fsr;
//    unsigned short gyro_rate, gyro_fsr;
//    unsigned long timestamp;
//    struct int_param_s int_param;

    /* Set up MSP430 hardware. */
//    platform_init();

    /* Set up gyro.
     * Every function preceded by mpu_ is a driver function and can be found
     * in inv_mpu.h.
     */
//    int_param.cb = gyro_data_ready_cb;
//    int_param.pin = INT_PIN_P20;
//    int_param.lp_exit = INT_EXIT_LPM0;
//    int_param.active_low = 1;
	
    result = mpu_init();// 0 if successful
    if (result)
	{
//        msp430_reset();
		//stm32_reset();
		return 1;
	}

    /* If you're not using an MPU9150 AND you're not using DMP features, this
     * function will place all slaves on the primary bus.
     * mpu_set_bypass(1);
     */

    /* Get/set hardware configuration. Start gyro. */
    /* Wake up all sensors. */
    if(mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL))	//开启所有传感器
	{
		return 2;
	}
    /* Push both gyro and accel data into the FIFO. */
    if(mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL))//配置FIFO来保存数据
	{
		return 3;
	}
    if(mpu_set_sample_rate(DEFAULT_MPU_HZ))				//设置MPU采样率, 这里为100Hz
	{													//当没使用DMP时, 则MPU的采样率由此设置决定; 当使用DMP时, 则由DMP的输出频率决定采样率
		return 4;
	}
	
    /* Read back configuration in case it was set improperly. */
//    mpu_get_sample_rate(&gyro_rate);
//    mpu_get_gyro_fsr(&gyro_fsr);//满量程
//    mpu_get_accel_fsr(&accel_fsr);

    /* Initialize HAL state variables. */
    memset(&hal, 0, sizeof(hal));					//用一堆0去初始化覆盖hal指向的内存
    hal.sensors = ACCEL_ON | GYRO_ON;				//保存gyro和accel的配置信息
//    hal.report = PRINT_QUAT;

    /* To initialize the DMP:
     * 1. Call dmp_load_motion_driver_firmware(). This pushes the DMP image in
     *    inv_mpu_dmp_motion_driver.h into the MPU memory.
     * 2. Push the gyro and accel orientation matrix to the DMP.
     * 3. Register gesture callbacks. Don't worry, these callbacks won't be
     *    executed unless the corresponding feature is enabled.
     * 4. Call dmp_enable_feature(mask) to enable different features.
     * 5. Call dmp_set_fifo_rate(freq) to select a DMP output rate.
     * 6. Call any feature-specific control functions.
     *
     * To enable the DMP, just call mpu_set_dmp_state(1). This function can
     * be called repeatedly to enable and disable the DMP at runtime.
     *
     * The following is a short summary of the features supported in the DMP
     * image provided in inv_mpu_dmp_motion_driver.c:
     * 		DMP_FEATURE_LP_QUAT: Generate a gyro-only quaternion on the DMP
     * at 200Hz. Integrating the gyro data at higher rates reduces numerical
     * errors (compared to integration on the MCU at a lower sampling rate).
     * 		DMP_FEATURE_6X_LP_QUAT: Generate a gyro/accel quaternion on the
     * DMP at 200Hz. Cannot be used in combination with DMP_FEATURE_LP_QUAT.
     * 		DMP_FEATURE_TAP: Detect taps along the X, Y, and Z axes.
     * 		DMP_FEATURE_ANDROID_ORIENT: Google's screen rotation algorithm.
     * Triggers an event at the four orientations where the screen should rotate.
     * 		DMP_FEATURE_GYRO_CAL: Calibrates the gyro data after eight seconds
     * of no motion.
     * 		DMP_FEATURE_SEND_RAW_ACCEL: Add raw accelerometer data to the FIFO.
     * 		DMP_FEATURE_SEND_RAW_GYRO: Add raw gyro data to the FIFO.
     * 		DMP_FEATURE_SEND_CAL_GYRO: Add calibrated gyro data to the FIFO. Cannot
     * be used in combination with DMP_FEATURE_SEND_RAW_GYRO.
     */
    if(dmp_load_motion_driver_firmware())
	{
		return 5;
	}
    if(dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation)))
	{
		return 6;
	}
//    dmp_register_tap_cb(tap_cb);							//注册手势点击事件
//    dmp_register_android_orient_cb(android_orient_cb);	//注册安卓设备方向事件
    hal.dmp_features = DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |
					DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL |
					DMP_FEATURE_SEND_CAL_GYRO | DMP_FEATURE_GYRO_CAL;
    if(dmp_enable_feature(hal.dmp_features))				//使能所用的dmp的特性功能
	{
		return 7;
	}
    if(dmp_set_fifo_rate(DEFAULT_MPU_HZ))					//dmp默认输出频率为100Hz
	{
		return 8;
	}
    mpu_set_dmp_state(1);			//开启dmp同时打开中断(默认高电平有效)
    hal.dmp_on = 1;					//保存dmp已开启的配置信息

	//开始自检, 并重新调零
	run_self_test();
	
	return 0;
	
//    __enable_interrupt();

    /* Wait for enumeration. */
//    while (USB_connectionState() != ST_ENUM_ACTIVE);
}



/**********************************************************************************
* 设置哪些数据想要上报给上位机
* rawDataReportCmd取值为:
* 		SET		则表示原始的gyro和accel数据均上报
*		RESET	则表示不上报二者
* quatDataReportCmd取值为:
* 		SET		则表示上报姿态角
*		RESET	则表示不上报
**********************************************************************************/
void SetReportFlag(u8 rawDataReportCmd, u8 quatDataReportCmd)
{
	if(rawDataReportCmd == SET)
	{
		hal.report |= PRINT_GYRO;
		hal.report |= PRINT_ACCEL;
	}
	else
	{
		hal.report &= ~PRINT_GYRO;
		hal.report &= ~PRINT_ACCEL;
	}
	
	if(quatDataReportCmd == SET)
		hal.report |= PRINT_QUAT;
	else
		hal.report &= ~PRINT_QUAT;
}





/**********************************************************************************
* dmp_getdata()用于获取数据, 当在外部调用此函数之后, 可以通过访问全局变量
* pitch, roll, yaw来获得姿态角, 以及gyro[3], accel[3]来获得原始数据
* return: 成功则返回0, 失败则返回1
**********************************************************************************/
//定义在读取dmp输出数据时常用的变量
unsigned long sensor_timestamp;				//时间戳
short gyro[3], accel[3], sensors;			//gyro-陀螺仪, accel-加速度计, sensors指示从FIFO中读到的数据是什么数据
unsigned char more;							//指示FIFO中是否还有剩余的数据
long quat[4];								//四元数(q30格式)
#define	q30 	1073741824.0f				//q30 = 2^30 (float)
float q0=1.0f, q1=0.0f, q2=0.0f, q3=0.0f;	//除去q30后的四元数临时值
float pitch, roll, yaw;						//姿态角


//获取数据给外部调用
u8 mpu_dmp_getdata(void)
{
	//判断上位机是都发送了新的数据
//	if (rx_new)
//		/* A byte has been received via USB. See handle_input for a list of
//		 * valid commands.
//		 */
//		handle_input();
//	msp430_get_clock_ms(&timestamp);

	//当运动中断模式开启时
//	if (hal.motion_int_mode) {
//		/* Enable motion interrupt. */
//		mpu_lp_motion_interrupt(500, 1, 5);				//低功耗中断配置
//		hal.new_gyro = 0;								//现将指示采样到新数据的标志变量复位, 以便等待INT中断
//		/* Wait for the MPU interrupt. */
//		while (!hal.new_gyro)
//			__bis_SR_register(LPM0_bits + GIE);
//		/* Restore the previous sensor configuration. */
//		mpu_lp_motion_interrupt(0, 0, 0);
//		hal.motion_int_mode = 0;
//	}

	//当没有新数据被采样或者DMP转换出来时
//	if (!hal.sensors || !hal.new_gyro) {
//		/* Put the MSP430 to sleep until a timer interrupt or data ready
//		 * interrupt is detected.
//		 */
//		__bis_SR_register(LPM0_bits + GIE);
//		continue;
//	}

	//使用了DMP功能, 可以获得原始数据和四元数数据
	if (hal.new_gyro && hal.dmp_on) {
		/* This function gets new data from the FIFO when the DMP is in
		 * use. The FIFO can contain any combination of gyro, accel,
		 * quaternion, and gesture data. The sensors parameter tells the
		 * caller which data fields were actually populated with new data.
		 * For example, if sensors == (INV_XYZ_GYRO | INV_WXYZ_QUAT), then
		 * the FIFO isn't being filled with accel data.
		 * The driver parses the gesture data to determine if a gesture
		 * event has occurred; on an event, the application will be notified
		 * via a callback (assuming that a callback function was properly
		 * registered). The more parameter is non-zero if there are
		 * leftover packets in the FIFO.
		 */
		//获得原始数据
		if(dmp_read_fifo(gyro, accel, quat, &sensor_timestamp, &sensors, &more))
		{
			return 1;
		}
		
		//当FIFO中已经没有剩余数据时, 将指示剩余数据量的more标志变量复位
		if (!more)
			hal.new_gyro = 0;
		
		/* Gyro and accel data are written to the FIFO by the DMP in chip
		 * frame and hardware units. This behavior is convenient because it
		 * keeps the gyro and accel outputs of dmp_read_fifo and
		 * mpu_read_fifo consistent.
		 * 表明gyro和accel的原始数据都是以芯片自身的框架坐标系为基准来采样的, 从而使用
		 * dmp_read_fifo()和mpu_read_fifo()读到的数据都是一致的, 并且注意: 当DMP没开启时
		 * 应使用mpu_read_fifo(), 而当开启了DMP后则使用dmp_read_fifo()读取数据
		 */
//		if (sensors & INV_XYZ_GYRO && hal.report & PRINT_GYRO)
//			send_packet(PACKET_TYPE_GYRO, gyro);
//		if (sensors & INV_XYZ_ACCEL && hal.report & PRINT_ACCEL)
//			send_packet(PACKET_TYPE_ACCEL, accel);
		
		/* Unlike gyro and accel, quaternions are written to the FIFO in
		 * the body frame, q30. The orientation is set by the scalar passed
		 * to dmp_set_orientation during initialization.
		 * 四元数是以设备安装的坐标系(方向矩阵)作为基准来获得数据的
		 */
//		if (sensors & INV_WXYZ_QUAT && hal.report & PRINT_QUAT)
//			send_packet(PACKET_TYPE_QUAT, quat);
	
		if(sensors & INV_WXYZ_QUAT)//如果获取到了四元数
		{
			q0 = quat[0] / q30;
			q1 = quat[1] / q30;
			q2 = quat[2] / q30;
			q3 = quat[3] / q30;
			
			//因为MPU6050的欧拉角和平常的不同, 它是pitch绕Y轴, roll绕X轴
			//所以为了保证PCB原先设定的飞机正方向, 下方只能强行交换pitch和roll的位置
			//平常情况下四元数转欧拉角应是 Pitch = asin, Roll = atan2, 而MPU是反的
			//除非焊接的时候旋转一下再焊MPU, 不然如果不交换pitch和roll, 机头方向就乱了
//			pitch = asin(2*q0*q2 - 2*q1*q3) * 57.3;
//			roll = atan2(2*q2*q3 + 2*q0*q1,  -2*q1*q1 - 2*q2*q2 + 1) * 57.3;
			roll = asin(2*q0*q2 - 2*q1*q3) * 57.3;								//for MPU
			pitch = atan2(2*q2*q3 + 2*q0*q1,  -2*q1*q1 - 2*q2*q2 + 1) * 57.3;	//for MPU
			yaw  = atan2(2*(q1*q2 + q0*q3),  q0*q0 + q1*q1 - q2*q2 - q3*q3) * 57.3;
		}
		
		//数据读取成功, 返回0
		return 0;		
	}
	
	//如果没有启用dmp
	else if (hal.new_gyro)
	{
		unsigned char sensors, more;
		/* This function gets new data from the FIFO. The FIFO can contain
		 * gyro, accel, both, or neither. The sensors parameter tells the
		 * caller which data fields were actually populated with new data.
		 * For example, if sensors == INV_XYZ_GYRO, then the FIFO isn't
		 * being filled with accel data. The more parameter is non-zero if
		 * there are leftover packets in the FIFO.
		 */
		if(mpu_read_fifo(gyro, accel, &sensor_timestamp, &sensors, &more))
		{
			//数据读取失败
			return 1;
		}
		if (!more)
			hal.new_gyro = 0;
//		if (sensors & INV_XYZ_GYRO && hal.report & PRINT_GYRO)
//			send_packet(PACKET_TYPE_GYRO, gyro);
//		if (sensors & INV_XYZ_ACCEL && hal.report & PRINT_ACCEL)
//			send_packet(PACKET_TYPE_ACCEL, accel);
		return 0;
	}
	
	//数据读取失败
	return 1;
}


//MPU_INT中断脚配置
void MPU_INT_Config(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//使能GPIOA的时钟
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;	//下拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);			//PA2
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2);
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line2;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;	//上升沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}









//串口1发送1个字符 
//c:要发送的字符
void usart1_send_char(u8 c)
{   	
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET); //循环发送,直到发送完毕   
	USART_SendData(USART1,c);  
}


//传送数据给匿名四轴上位机软件(V2.6版本)
//fun:功能字. 0XA0~0XAF
//data:数据缓存区,最多28字节!!
//len:data区有效数据个数
void usart1_niming_report(u8 fun,u8*data,u8 len)
{
	u8 send_buf[32];
	u8 i;
	if(len>28)return;	//最多28字节数据 
	send_buf[len+3]=0;	//校验数置零
	send_buf[0]=0X88;	//帧头
	send_buf[1]=fun;	//功能字
	send_buf[2]=len;	//数据长度
	for(i=0;i<len;i++)send_buf[3+i]=data[i];			//复制数据
	for(i=0;i<len+3;i++)send_buf[len+3]+=send_buf[i];	//计算校验和	
	for(i=0;i<len+4;i++)usart1_send_char(send_buf[i]);	//发送数据到串口1 
}


//通过串口1上报结算后的姿态数据给电脑
//aacx,aacy,aacz:x,y,z三个方向上面的加速度值
//gyrox,gyroy,gyroz:x,y,z三个方向上面的陀螺仪值
//roll:横滚角.单位0.01度。 -18000 -> 18000 对应 -180.00  ->  180.00度
//pitch:俯仰角.单位 0.01度。-9000 - 9000 对应 -90.00 -> 90.00 度
//yaw:航向角.单位为0.1度 0 -> 3600  对应 0 -> 360.0度
void usart1_report_imu(int16_t aacx,  int16_t aacy,  int16_t aacz,  int16_t gyrox,  int16_t gyroy,\
					  int16_t gyroz,  int16_t roll,  int16_t pitch,  int16_t yaw)
{
	u8 tbuf[28]; 
	u8 i;
	for(i=0;i<28;i++)tbuf[i]=0;//清0
	tbuf[0]=(aacx>>8)&0XFF;
	tbuf[1]=aacx&0XFF;
	tbuf[2]=(aacy>>8)&0XFF;
	tbuf[3]=aacy&0XFF;
	tbuf[4]=(aacz>>8)&0XFF;
	tbuf[5]=aacz&0XFF; 
	tbuf[6]=(gyrox>>8)&0XFF;
	tbuf[7]=gyrox&0XFF;
	tbuf[8]=(gyroy>>8)&0XFF;
	tbuf[9]=gyroy&0XFF;
	tbuf[10]=(gyroz>>8)&0XFF;
	tbuf[11]=gyroz&0XFF;	
	tbuf[18]=(roll>>8)&0XFF;
	tbuf[19]=roll&0XFF;
	tbuf[20]=(pitch>>8)&0XFF;
	tbuf[21]=pitch&0XFF;
	tbuf[22]=(yaw>>8)&0XFF;
	tbuf[23]=yaw&0XFF;
	usart1_niming_report(0XAF,tbuf,28);//飞控显示帧,0XAF
}







u8 MPU_DMP_RDY = 0;



//MPU_INT中断(PA2)
void EXTI2_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line2)!=RESET)
	{	  
		MPU_DMP_RDY = 1;
		
		gyro_data_ready_cb();
		//while(mpu_dmp_getdata() != 0);//如果读取FIFO失败就立刻继续读取直至成功
		//usart1_report_imu(accel[0],  accel[1],  accel[2],  gyro[0],  gyro[1],  gyro[2],\
							(int16_t)(roll*100),  (int16_t)(pitch*100),  (int16_t)(yaw*10));
	}
	EXTI_ClearITPendingBit(EXTI_Line2);
}












//发送加速度传感器数据和陀螺仪数据
//aacx,aacy,aacz:x,y,z三个方向上面的加速度值
//gyrox,gyroy,gyroz:x,y,z三个方向上面的陀螺仪值
void mpu6050_send_data(short aacx,short aacy,short aacz,short gyrox,short gyroy,short gyroz)
{
	u8 tbuf[12]; 
	tbuf[0]=(aacx>>8)&0XFF;
	tbuf[1]=aacx&0XFF;
	tbuf[2]=(aacy>>8)&0XFF;
	tbuf[3]=aacy&0XFF;
	tbuf[4]=(aacz>>8)&0XFF;
	tbuf[5]=aacz&0XFF; 
	tbuf[6]=(gyrox>>8)&0XFF;
	tbuf[7]=gyrox&0XFF;
	tbuf[8]=(gyroy>>8)&0XFF;
	tbuf[9]=gyroy&0XFF;
	tbuf[10]=(gyroz>>8)&0XFF;
	tbuf[11]=gyroz&0XFF;
	usart1_niming_report(0XA1,tbuf,12);//自定义帧,0XA1
}
