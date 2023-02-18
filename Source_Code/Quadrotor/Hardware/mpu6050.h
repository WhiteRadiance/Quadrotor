#ifndef   _MPU6050_H
#define   _MPU6050_H

#include "stm32f10x.h"

#define MPU6050Addr 	0x68		//AD0接地:110_100_0,再左移一位得到0xD0

//三维整形
typedef struct{
	int16_t	X;
	int16_t	Y;
	int16_t	Z;
}INT16_XYZ;

//三维浮点型
typedef struct{
	float	X;
	float	Y;
	float	Z;
}FLOAT_XYZ;

//姿态结算后的欧拉角
typedef struct{
	float	rol;
	float	yaw;
	float	pit;
}FLOAT_ANGLE;






#define MPU6050_RA_SMPLRT_DIV       0x19
#define MPU6050_RA_CONFIG           0x1A
#define MPU6050_RA_GYRO_CONFIG      0x1B
#define MPU6050_RA_ACCEL_CONFIG     0x1C


#define MPU6050_RA_INT_PIN_CFG      0x37
#define MPU6050_RA_INT_ENABLE       0x38


#define MPU6050_RA_ACCEL_XOUT_H     0x3B
#define MPU6050_RA_ACCEL_XOUT_L     0x3C
#define MPU6050_RA_ACCEL_YOUT_H     0x3D
#define MPU6050_RA_ACCEL_YOUT_L     0x3E
#define MPU6050_RA_ACCEL_ZOUT_H     0x3F
#define MPU6050_RA_ACCEL_ZOUT_L     0x40
#define MPU6050_RA_TEMP_OUT_H       0x41
#define MPU6050_RA_TEMP_OUT_L       0x42
#define MPU6050_RA_GYRO_XOUT_H      0x43
#define MPU6050_RA_GYRO_XOUT_L      0x44
#define MPU6050_RA_GYRO_YOUT_H      0x45
#define MPU6050_RA_GYRO_YOUT_L      0x46
#define MPU6050_RA_GYRO_ZOUT_H      0x47
#define MPU6050_RA_GYRO_ZOUT_L      0x48


#define MPU6050_RA_PWR_MGMT_1       0x6B
#define MPU6050_RA_PWR_MGMT_2       0x6C
#define MPU6050_RA_WHO_AM_I         0x75


//设置低通滤波器
#define MPU6050_DLPF_BW_256         0x00
#define MPU6050_DLPF_BW_188         0x01
#define MPU6050_DLPF_BW_98          0x02
#define MPU6050_DLPF_BW_42          0x03
#define MPU6050_DLPF_BW_20          0x04
#define MPU6050_DLPF_BW_10          0x05
#define MPU6050_DLPF_BW_5           0x06



//MPU6050.h 对 I2C.h读写函数的ADDR参数进行简单封装
u8 MPU6050_WriteByte(u8 REG, u8 data);
u8 MPU6050_ReadByte(u8 REG, u8* pdata);
u8 MPU6050_WriteBytes(u8 REG, u8 LEN, u8* Wr_BUF);
u8 MPU6050_ReadBytes(u8 REG, u8 LEN, u8* Rd_BUF);


//常用函数
u8 MPU6050_Init(void);
//void MPU6050_Check(void);
void MPU6050_GetRawData(void);
//void MPU6050_Offset(void);
//void MPU6050_CalOff(void);
//void MPU6050_CalOff_Acc(void);
//void MPU6050_CalOff_Gyr(void);
//void MPU6050_GyroRead(int16_t *gyroData);
//void MPU6050_AccRead(int16_t *accData);
//void MPU6050_TempRead(float *tempdata);
//u8	MPU6050_testConnection(void);
void RawData_Processing(void);
void IMUupdate(FLOAT_XYZ *Gyr_rad, FLOAT_XYZ *Acc_filt, FLOAT_ANGLE *Euler_Angle);

#endif

