#include "mpu_dmp_api.h"
/*
	USART_Config();
	
	//���ж����ȼ�����
	NVIC_PriorityConfig();
	
	//��ʼ��i2c, ����systick��delay��ʼ��
	I2C_Config();
	
	//��ʼ��MPU
	mpu_dmp_init();
	
	//������Щ�����ϱ�����λ��
	SetReportFlag(SET,SET);
	
	//����DMP�ж�����źŵ�EXTI��ʼ��
	EXTI_Config();
	
	while(1){
		//��ʼѭ����ȡ����, ���ϱ����ݸ���λ��
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
#define PRINT_ACCEL     (0x01)			//����λ���ϱ����ټ����ݵı�־
#define PRINT_GYRO      (0x02)			//����λ���ϱ����������ݵı�־
#define PRINT_QUAT      (0x04)			//����λ���ϱ���Ԫ�����ݵı�־

#define ACCEL_ON        (0x01)			//ָʾ���ټ��Ѿ��򿪵ı�־
#define GYRO_ON         (0x02)			//ָʾ�������Ѿ��򿪵ı�־

#define MOTION          (0)				//ָʾMPU�����˶�״̬
#define NO_MOTION       (1)				//ָʾMPU���ھ�ֹ״̬

/* Starting sampling rate. */
#define DEFAULT_MPU_HZ  (100)

#define FLASH_SIZE      (512)
#define FLASH_MEM_START ((void*)0x1800)

struct rx_s {
    unsigned char header[3];
    unsigned char cmd;
};
struct hal_s {
    unsigned char sensors;				//����ָʾ�õ�����Щ������
    unsigned char dmp_on;				//����ָʾ����/�ر�DMP
    unsigned char wait_for_tap;			//�ȴ�����/�û�����
    volatile unsigned char new_gyro;	//ָʾ�Ƿ����������Ѿ�׼������
    unsigned short report;				//����ָʾ�Ƿ��ϱ����ݸ���λ��
    unsigned short dmp_features;		//����ָʾ������DMP����Щ����
    unsigned char motion_int_mode;		//����ָʾ�˶��ж��Ƿ��
    struct rx_s rx;						//���ⲿ���յ�������
};
static struct hal_s hal = {0};			//��ʼ��hal, ȫ����Ա����λΪ0

/* USB RX binary semaphore. Actually, it's just a flag. Not included in struct
 * because it's declared extern elsewhere.
 */
volatile unsigned char rx_new;			//����ָʾ��û�н��յ��ⲿ�´���������

/* �������
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

//���ƻص�����
//static void tap_cb(unsigned char direction, unsigned char count)
//{
//    char data[2];
//    data[0] = (char)direction;
//    data[1] = (char)count;
//    send_packet(PACKET_TYPE_TAP, data);
//}

//��׿ϵͳ�ķ���ص�����
//static void android_orient_cb(unsigned char orientation)
//{
//    send_packet(PACKET_TYPE_ANDROID_ORIENT, &orientation);
//}

//����msp430
//static inline void msp430_reset(void)
//{
//    PMMCTL0 |= PMMSWPOR;
//}

//����stm32
void stm32_reset(void)
{
	__set_FAULTMASK(1);	// 0:�������ж�     1:�ر������ж�
	NVIC_SystemReset();	// ϵͳ��λ
}

//�Լ� �� У��ƫ��
void run_self_test(void)
{
    int result;
//    char test_packet[4] = {0};
    long gyro[3], accel[3];

    result = mpu_run_self_test(gyro, accel);
    //result=7����MPU6500, =3����MPU6050
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
		
		//accel_sens = 0;//����˾仰��������ɻ�б�ŷŽ���У׼
		
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

//Invensense�ٷ���λ���Էɻ�����У׼����
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



//������������׼����
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




//��ʼ��MPU6050, ���Լ��У׼
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
    if(mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL))	//�������д�����
	{
		return 2;
	}
    /* Push both gyro and accel data into the FIFO. */
    if(mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL))//����FIFO����������
	{
		return 3;
	}
    if(mpu_set_sample_rate(DEFAULT_MPU_HZ))				//����MPU������, ����Ϊ100Hz
	{													//��ûʹ��DMPʱ, ��MPU�Ĳ������ɴ����þ���; ��ʹ��DMPʱ, ����DMP�����Ƶ�ʾ���������
		return 4;
	}
	
    /* Read back configuration in case it was set improperly. */
//    mpu_get_sample_rate(&gyro_rate);
//    mpu_get_gyro_fsr(&gyro_fsr);//������
//    mpu_get_accel_fsr(&accel_fsr);

    /* Initialize HAL state variables. */
    memset(&hal, 0, sizeof(hal));					//��һ��0ȥ��ʼ������halָ����ڴ�
    hal.sensors = ACCEL_ON | GYRO_ON;				//����gyro��accel��������Ϣ
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
//    dmp_register_tap_cb(tap_cb);							//ע�����Ƶ���¼�
//    dmp_register_android_orient_cb(android_orient_cb);	//ע�ᰲ׿�豸�����¼�
    hal.dmp_features = DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |
					DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL |
					DMP_FEATURE_SEND_CAL_GYRO | DMP_FEATURE_GYRO_CAL;
    if(dmp_enable_feature(hal.dmp_features))				//ʹ�����õ�dmp�����Թ���
	{
		return 7;
	}
    if(dmp_set_fifo_rate(DEFAULT_MPU_HZ))					//dmpĬ�����Ƶ��Ϊ100Hz
	{
		return 8;
	}
    mpu_set_dmp_state(1);			//����dmpͬʱ���ж�(Ĭ�ϸߵ�ƽ��Ч)
    hal.dmp_on = 1;					//����dmp�ѿ�����������Ϣ

	//��ʼ�Լ�, �����µ���
	run_self_test();
	
	return 0;
	
//    __enable_interrupt();

    /* Wait for enumeration. */
//    while (USB_connectionState() != ST_ENUM_ACTIVE);
}



/**********************************************************************************
* ������Щ������Ҫ�ϱ�����λ��
* rawDataReportCmdȡֵΪ:
* 		SET		���ʾԭʼ��gyro��accel���ݾ��ϱ�
*		RESET	���ʾ���ϱ�����
* quatDataReportCmdȡֵΪ:
* 		SET		���ʾ�ϱ���̬��
*		RESET	���ʾ���ϱ�
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
* dmp_getdata()���ڻ�ȡ����, �����ⲿ���ô˺���֮��, ����ͨ������ȫ�ֱ���
* pitch, roll, yaw�������̬��, �Լ�gyro[3], accel[3]�����ԭʼ����
* return: �ɹ��򷵻�0, ʧ���򷵻�1
**********************************************************************************/
//�����ڶ�ȡdmp�������ʱ���õı���
unsigned long sensor_timestamp;				//ʱ���
short gyro[3], accel[3], sensors;			//gyro-������, accel-���ٶȼ�, sensorsָʾ��FIFO�ж�����������ʲô����
unsigned char more;							//ָʾFIFO���Ƿ���ʣ�������
long quat[4];								//��Ԫ��(q30��ʽ)
#define	q30 	1073741824.0f				//q30 = 2^30 (float)
float q0=1.0f, q1=0.0f, q2=0.0f, q3=0.0f;	//��ȥq30�����Ԫ����ʱֵ
float pitch, roll, yaw;						//��̬��


//��ȡ���ݸ��ⲿ����
u8 mpu_dmp_getdata(void)
{
	//�ж���λ���Ƕ��������µ�����
//	if (rx_new)
//		/* A byte has been received via USB. See handle_input for a list of
//		 * valid commands.
//		 */
//		handle_input();
//	msp430_get_clock_ms(&timestamp);

	//���˶��ж�ģʽ����ʱ
//	if (hal.motion_int_mode) {
//		/* Enable motion interrupt. */
//		mpu_lp_motion_interrupt(500, 1, 5);				//�͹����ж�����
//		hal.new_gyro = 0;								//�ֽ�ָʾ�����������ݵı�־������λ, �Ա�ȴ�INT�ж�
//		/* Wait for the MPU interrupt. */
//		while (!hal.new_gyro)
//			__bis_SR_register(LPM0_bits + GIE);
//		/* Restore the previous sensor configuration. */
//		mpu_lp_motion_interrupt(0, 0, 0);
//		hal.motion_int_mode = 0;
//	}

	//��û�������ݱ���������DMPת������ʱ
//	if (!hal.sensors || !hal.new_gyro) {
//		/* Put the MSP430 to sleep until a timer interrupt or data ready
//		 * interrupt is detected.
//		 */
//		__bis_SR_register(LPM0_bits + GIE);
//		continue;
//	}

	//ʹ����DMP����, ���Ի��ԭʼ���ݺ���Ԫ������
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
		//���ԭʼ����
		if(dmp_read_fifo(gyro, accel, quat, &sensor_timestamp, &sensors, &more))
		{
			return 1;
		}
		
		//��FIFO���Ѿ�û��ʣ������ʱ, ��ָʾʣ����������more��־������λ
		if (!more)
			hal.new_gyro = 0;
		
		/* Gyro and accel data are written to the FIFO by the DMP in chip
		 * frame and hardware units. This behavior is convenient because it
		 * keeps the gyro and accel outputs of dmp_read_fifo and
		 * mpu_read_fifo consistent.
		 * ����gyro��accel��ԭʼ���ݶ�����оƬ����Ŀ������ϵΪ��׼��������, �Ӷ�ʹ��
		 * dmp_read_fifo()��mpu_read_fifo()���������ݶ���һ�µ�, ����ע��: ��DMPû����ʱ
		 * Ӧʹ��mpu_read_fifo(), ����������DMP����ʹ��dmp_read_fifo()��ȡ����
		 */
//		if (sensors & INV_XYZ_GYRO && hal.report & PRINT_GYRO)
//			send_packet(PACKET_TYPE_GYRO, gyro);
//		if (sensors & INV_XYZ_ACCEL && hal.report & PRINT_ACCEL)
//			send_packet(PACKET_TYPE_ACCEL, accel);
		
		/* Unlike gyro and accel, quaternions are written to the FIFO in
		 * the body frame, q30. The orientation is set by the scalar passed
		 * to dmp_set_orientation during initialization.
		 * ��Ԫ�������豸��װ������ϵ(�������)��Ϊ��׼��������ݵ�
		 */
//		if (sensors & INV_WXYZ_QUAT && hal.report & PRINT_QUAT)
//			send_packet(PACKET_TYPE_QUAT, quat);
	
		if(sensors & INV_WXYZ_QUAT)//�����ȡ������Ԫ��
		{
			q0 = quat[0] / q30;
			q1 = quat[1] / q30;
			q2 = quat[2] / q30;
			q3 = quat[3] / q30;
			
			//��ΪMPU6050��ŷ���Ǻ�ƽ���Ĳ�ͬ, ����pitch��Y��, roll��X��
			//����Ϊ�˱�֤PCBԭ���趨�ķɻ�������, �·�ֻ��ǿ�н���pitch��roll��λ��
			//ƽ���������Ԫ��תŷ����Ӧ�� Pitch = asin, Roll = atan2, ��MPU�Ƿ���
			//���Ǻ��ӵ�ʱ����תһ���ٺ�MPU, ��Ȼ���������pitch��roll, ��ͷ���������
//			pitch = asin(2*q0*q2 - 2*q1*q3) * 57.3;
//			roll = atan2(2*q2*q3 + 2*q0*q1,  -2*q1*q1 - 2*q2*q2 + 1) * 57.3;
			roll = asin(2*q0*q2 - 2*q1*q3) * 57.3;								//for MPU
			pitch = atan2(2*q2*q3 + 2*q0*q1,  -2*q1*q1 - 2*q2*q2 + 1) * 57.3;	//for MPU
			yaw  = atan2(2*(q1*q2 + q0*q3),  q0*q0 + q1*q1 - q2*q2 - q3*q3) * 57.3;
		}
		
		//���ݶ�ȡ�ɹ�, ����0
		return 0;		
	}
	
	//���û������dmp
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
			//���ݶ�ȡʧ��
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
	
	//���ݶ�ȡʧ��
	return 1;
}


//MPU_INT�жϽ�����
void MPU_INT_Config(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//ʹ��GPIOA��ʱ��
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;	//����
	GPIO_Init(GPIOA, &GPIO_InitStructure);			//PA2
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2);
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line2;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;	//�����ش���
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}









//����1����1���ַ� 
//c:Ҫ���͵��ַ�
void usart1_send_char(u8 c)
{   	
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET); //ѭ������,ֱ���������   
	USART_SendData(USART1,c);  
}


//�������ݸ�����������λ�����(V2.6�汾)
//fun:������. 0XA0~0XAF
//data:���ݻ�����,���28�ֽ�!!
//len:data����Ч���ݸ���
void usart1_niming_report(u8 fun,u8*data,u8 len)
{
	u8 send_buf[32];
	u8 i;
	if(len>28)return;	//���28�ֽ����� 
	send_buf[len+3]=0;	//У��������
	send_buf[0]=0X88;	//֡ͷ
	send_buf[1]=fun;	//������
	send_buf[2]=len;	//���ݳ���
	for(i=0;i<len;i++)send_buf[3+i]=data[i];			//��������
	for(i=0;i<len+3;i++)send_buf[len+3]+=send_buf[i];	//����У���	
	for(i=0;i<len+4;i++)usart1_send_char(send_buf[i]);	//�������ݵ�����1 
}


//ͨ������1�ϱ���������̬���ݸ�����
//aacx,aacy,aacz:x,y,z������������ļ��ٶ�ֵ
//gyrox,gyroy,gyroz:x,y,z�������������������ֵ
//roll:�����.��λ0.01�ȡ� -18000 -> 18000 ��Ӧ -180.00  ->  180.00��
//pitch:������.��λ 0.01�ȡ�-9000 - 9000 ��Ӧ -90.00 -> 90.00 ��
//yaw:�����.��λΪ0.1�� 0 -> 3600  ��Ӧ 0 -> 360.0��
void usart1_report_imu(int16_t aacx,  int16_t aacy,  int16_t aacz,  int16_t gyrox,  int16_t gyroy,\
					  int16_t gyroz,  int16_t roll,  int16_t pitch,  int16_t yaw)
{
	u8 tbuf[28]; 
	u8 i;
	for(i=0;i<28;i++)tbuf[i]=0;//��0
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
	usart1_niming_report(0XAF,tbuf,28);//�ɿ���ʾ֡,0XAF
}







u8 MPU_DMP_RDY = 0;



//MPU_INT�ж�(PA2)
void EXTI2_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line2)!=RESET)
	{	  
		MPU_DMP_RDY = 1;
		
		gyro_data_ready_cb();
		//while(mpu_dmp_getdata() != 0);//�����ȡFIFOʧ�ܾ����̼�����ȡֱ���ɹ�
		//usart1_report_imu(accel[0],  accel[1],  accel[2],  gyro[0],  gyro[1],  gyro[2],\
							(int16_t)(roll*100),  (int16_t)(pitch*100),  (int16_t)(yaw*10));
	}
	EXTI_ClearITPendingBit(EXTI_Line2);
}












//���ͼ��ٶȴ��������ݺ�����������
//aacx,aacy,aacz:x,y,z������������ļ��ٶ�ֵ
//gyrox,gyroy,gyroz:x,y,z�������������������ֵ
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
	usart1_niming_report(0XA1,tbuf,12);//�Զ���֡,0XA1
}
