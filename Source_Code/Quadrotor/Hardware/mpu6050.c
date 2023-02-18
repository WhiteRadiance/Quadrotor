#include "mpu6050.h"
#include "i2c.h"
#include "delay.h"
#include <math.h>

#define Kp_New      0.8f              //�����˲���ǰ���ݵ�Ȩ��
#define Kp_Old      0.2f              //�����˲���ʷ���ݵ�Ȩ��

#define	Gravity		9.80866f
#define	Rad2Deg		57.2958f
#define Deg2Rad		0.0174533f

#define ACCEL2Gravity	0.0001211f	//ת�������̵�λ g		+-8g->0.0002441f	+-4g->0.0001211f	+-2g->0.0000610f
#define GYRO2DegPerSec	0.0610361f	//ת�������̵�λ deg/s	+-2000->0.0610361f	+-1000->0.0305180f
#define	GYRO2RadPerSec	0.0010653f	//ֱ��ת���ɵ�λ rad/s	+-2000->0.0010653f	+-1000->0.0005326f

uint8_t		mpu6050_ID;
int16_t		MPU_TEMP;
float		mpu_temperature;
INT16_XYZ	ACCEL_RAW, GYRO_RAW;
FLOAT_XYZ	Gyro_degree, Gyro_rad;
FLOAT_XYZ	/*Gyro_filt, */Accel_filt, Accel_filt_old;
FLOAT_ANGLE Angle;


//��IIC���������ٷ�װ, ������ַ�����ǲ�����
u8 MPU6050_WriteByte(u8 REG, u8 data)
{
	u8 error = I2C_WriteByte(MPU6050Addr, REG, data);
	return error;
}
u8 MPU6050_ReadByte(u8 REG, u8* pdata)
{
	u8 error = I2C_ReadByte(MPU6050Addr, REG, pdata);
	return error;
}
u8 MPU6050_WriteBytes(u8 REG, u8 LEN, u8* Wr_BUF)
{
	u8 error = I2C_WriteBytes(MPU6050Addr, REG, LEN, Wr_BUF);
	return error;
}
u8 MPU6050_ReadBytes(u8 REG, u8 LEN, u8* Rd_BUF)
{
	u8 error = I2C_ReadBytes(MPU6050Addr, REG, LEN, Rd_BUF);
	return error;
}

//return 0 if OK, or return 1 if failed.
u8 MPU6050_Init(void)
{
	IIC_Init();
	
	delay_ms(200);
	MPU6050_WriteByte(MPU6050_RA_PWR_MGMT_1, 0x80);	//��λMPU6050ȫ���Ĵ���
	delay_ms(200);
	MPU6050_WriteByte(MPU6050_RA_PWR_MGMT_1, 0x01);	//����MPU6050, ��ѡ��Gyro_X��PLL��Ϊʱ��Դ
	delay_ms(200);
	MPU6050_WriteByte(MPU6050_RA_INT_ENABLE, 0x00);	//��ֹ�����ж�
	MPU6050_WriteByte(MPU6050_RA_GYRO_CONFIG, 0x18);//������Gyro��������Ϊ ��2000��/s
	MPU6050_WriteByte(MPU6050_RA_ACCEL_CONFIG, 0x08);//���ټ�Accel��������Ϊ ��4g/s
	MPU6050_WriteByte(MPU6050_RA_CONFIG, 0x04);//�������ݵ����Ϊ1kHZ,DLPF=20Hz
	MPU6050_WriteByte(MPU6050_RA_SMPLRT_DIV, 0x13);//������Ƶ (����Ƶ�� = ���������Ƶ�� / (1+DIV)������Ƶ��50Hz)
	//MPU6050_WriteByte(MPU6050_RA_INT_PIN_CFG, 0x02); //MPU ��ֱ�ӷ���MPU6050����I2C
	
	MPU6050_ReadByte(MPU6050_RA_WHO_AM_I, &mpu6050_ID);//��ȡ����ID, ӦΪ0x68��ʵ�ʶ���ȴΪ0x98
	if(mpu6050_ID != 0x00)	return 0;
	else	return 1;
}	

//��ȡԭʼ����
void MPU6050_GetRawData(void)
{
	uint8_t	buffer[14];
	MPU6050_ReadBytes(0x3B, 14, buffer);
	
	ACCEL_RAW.X = (buffer[0]<<8) | buffer[1];		//16bit ����
	ACCEL_RAW.Y = (buffer[2]<<8) | buffer[3];
	ACCEL_RAW.Z = (buffer[4]<<8) | buffer[5];
	MPU_TEMP	= (buffer[6]<<8) | buffer[7];		//16bit ����	
	GYRO_RAW.X	= (buffer[8]<<8) | buffer[9];		//16bit ����
	GYRO_RAW.Y	= (buffer[10]<<8) | buffer[11];
	GYRO_RAW.Z	= (buffer[12]<<8) | buffer[13];
}

//��RAW���ݻ��㵥λ(������������)
void RawData_Processing(void)
{
	//SortAver_FilterXYZ(&MPU6050_ACC_RAW,&Acc_filt,12);//���� ��ֵ���������˲�
	Accel_filt.X = ACCEL_RAW.X - 300;
	Accel_filt.Y = ACCEL_RAW.Y + 660;
	Accel_filt.Z = ACCEL_RAW.Z - 400;
	
	//���ٶ�ADֵ ת���� g = m/(s*s)
	Accel_filt.X = (float)Accel_filt.X * ACCEL2Gravity * Gravity;
	Accel_filt.Y = (float)Accel_filt.Y * ACCEL2Gravity * Gravity;
	Accel_filt.Z = (float)Accel_filt.Z * ACCEL2Gravity * Gravity;

	//������ADֵ ת���� rad/s
	Gyro_rad.X = (float)GYRO_RAW.X * GYRO2RadPerSec;
	Gyro_rad.Y = (float)GYRO_RAW.Y * GYRO2RadPerSec;
	Gyro_rad.Z = (float)GYRO_RAW.Z * GYRO2RadPerSec;
	
	
	
	
	
	//�˴�����˲��㷨
	/*
		Accel_filt.X = Accel_filt.X * Kp_New + Accel_filt_old.X * Kp_Old;
		Accel_filt.Y = Accel_filt.Y * Kp_New + Accel_filt_old.Y * Kp_Old;
		Accel_filt.Z = Accel_filt.Z * Kp_New + Accel_filt_old.Z * Kp_Old;
		
		Accel_filt_old.X =  Accel_filt.X;
		Accel_filt_old.Y =  Accel_filt.Y;
		Accel_filt_old.Z =  Accel_filt.Z;
	*/
	
	
	
}

//kp=ki=0 ������ȫ���������ǵ�����
#define Kp 5.90f			// proportional gain governs rate of convergence to accelerometer/magnetometer
							// ����ϵ�� ���Ƽ��ټƺʹ����Ƶ������ٶ�
#define Ki 0.002f			// integral gain governs rate of convergence of gyroscope biases  
							// ����ϵ�� ���������ǵ�ƫ��������ٶ�
#define halfdeltaT 0.005f	// half of the sample period
							// �������ڵ�һ��
float q0=1.0f, q1=0.0f, q2=0.0f, q3=0.0f;	// quaternion elements representing the estimated orientation
float exInt=0, eyInt=0, ezInt=0;			// scaled integral error

//���ټ��� 1/Sqrt(x)
//����ͨ��Sqrt()����Ҫ���ı�
//����: http://en.wikipedia.org/wiki/Fast_inverse_square_root
static float invSqrt(float x) 
{
	float halfx = 0.5f * x;
	float y = x;
	long i = *(long*)&y;
	i = 0x5f375a86 - (i>>1);
	y = *(float*)&i;
	y = y * (1.5f - (halfx * y * y));
	return y;
}

//��Ԫ��
void IMUupdate(FLOAT_XYZ *Gyr_rad, FLOAT_XYZ *Acc_filt, FLOAT_ANGLE *Euler_Angle)
{
	//uint8_t i;
	//float matrix[9] = {1.f,  0.0f,  0.0f, 0.0f,  1.f,  0.0f, 0.0f,  0.0f,  1.f };//��ʼ������
	float ax=Acc_filt->X, ay=Acc_filt->Y, az=Acc_filt->Z;
	float gx=Gyr_rad->X, gy=Gyr_rad->Y, gz=Gyr_rad->Z;
	float vx, vy, vz;
	float ex, ey, ez;
	float norm;
	
	//��ǰ����Щ�����, �����ظ�����
	//float q0q0=q0*q0, q0q1=q0*q1, q0q2=q0*q2, q0q3=q0*q3, q1q1=q1*q1;
	//float q1q2=q1*q2, q1q3=q1*q3, q2q2=q2*q2, q2q3=q2*q3, q3q3=q3*q3;
	
	//����������ݶ���0, ����ټƹ�һ��ʱ����ֳ���0�Ĵ���
	if(ax*ay*az==0)	return;
	
	//���ټƵ���ά��λ���� acc
	//norm = invSqrt(ax*ax + ay*ay + az*az);
	norm = sqrt(ax*ax + ay*ay + az*az);//normalise the measurements
	ax = ax / norm;
	ay = ay / norm;
	az = az / norm;
	
	//��ȡ��Ԫ����Ч���Ҿ����е��������� vector_gravity
	//ע: �������Ҿ�������е�����Ԫ�ؼ�T13,T23,T33(���߲�ת����Ϊ��������)
	vx = 2*(q1*q3 - q0*q2);//estimate direction of gravity
	vy = 2*(q0*q1 + q2*q3);
	vz = q0*q0 - q1*q1 - q2*q2 + q3*q3;
	
	//acc��vector_gravity������������������������Ĳ�ƽ�г̶�, ���ģ=|a||b|sin�������Ƕȳ�����
	//�������ǰ����������ת��������������, ��Ȼʵ��ת���ĽǶ��ǰ�ĳ������ȥ����accel����gyro��
	// error is sum of cross product between reference direction of field and direction measured by sensor
	ex = ay*vz - az*vy;	//+ (my*wz - mz*wy);
	ey = az*vx - ax*vz;	//+ (mz*wx - mx*wz);
	ex = ax*vy - ay*vx;	//+ (mx*wy - my*wx);
	
	//�����ϵ������л���(���)
	exInt = exInt + ex * Ki ;// integral error scaled integral gain
	eyInt = eyInt + ey * Ki ;
	ezInt = ezInt + ez * Ki ;
	
	//�����˲�, ���������������ٶ���, �������ٶȻ���Ư��
	gx = gx + Kp * ex + exInt;// adjusted gyroscope measurements
	gy = gy + Kp * ey + eyInt;
	gz = gz + Kp * ez + ezInt;//�˴���gz����û�д�����У��, �����Ư��(�����������Լ�)
	
	//һ�������������ȡ��Ԫ��q0q1q2q3
	// integrate quaternion rate and normalise
	q0 = q0 + (-q1*gx - q2*gy - q3*gz)*halfdeltaT;
	q1 = q1 + ( q0*gx + q2*gz - q3*gy)*halfdeltaT;
	q2 = q2 + ( q0*gy - q1*gz + q3*gx)*halfdeltaT;
	q3 = q3 + ( q0*gz + q1*gy - q2*gx)*halfdeltaT;
	
	//���뵥λ����Ԫ���ſ��Լ���ת��ŷ����
	//norm = invSqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
	norm = sqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);// normalise quaternion
	q0 = q0 / norm;
	q1 = q1 / norm;
	q2 = q2 / norm;
	q3 = q3 / norm;
	
	//����R ����������ϵ(n)ת������������ϵ(b), ���ڶ���
//	matrix[0] = q0q0 + q1q1 - q2q2 - q3q3;	// 11(ǰ�к���)
//	matrix[1] = 2.f * (q1q2 + q0q3);	    // 12
//	matrix[2] = 2.f * (q1q3 - q0q2);	    // 13
//	matrix[3] = 2.f * (q1q2 - q0q3);	    // 21
//	matrix[4] = q0q0 - q1q1 + q2q2 - q3q3;	// 22
//	matrix[5] = 2.f * (q2q3 + q0q1);	    // 23
//	matrix[6] = 2.f * (q1q3 + q0q2);	    // 31
//	matrix[7] = 2.f * (q2q3 - q0q1);	    // 32
//	matrix[8] = q0q0 - q1q1 - q2q2 + q3q3;	// 33
	
	//��Ԫ��ת����ŷ����(Z->Y->X)
	Euler_Angle->pit = asin(2*(q0*q2 - q1*q3)) * 57.296f;						// ����ԭ��pitch
	//Euler_Angle->pit = asin(2*(q0q1 + q2q3)) * 57.296f;							//�ҵ�
	
	Euler_Angle->rol = atan2(2*(q0*q1 + q2*q3), 1 - 2*q1*q1 - 2*q2*q2)* 57.296f;	// ����ԭ��roll ���ֺܲ�, ����
	//Euler_Angle->rol = atan2(2*(q0q2 - q1q3), q0q0 - q1q1 - q2q2 + q3q3)* 57.296f;	//�ҵ�	
	
	
	
	//Euler_Angle->yaw += gz * Rad2Deg * 0.01f;	//ֱ�Ӷ������ǻ���
	//Euler_Angle->yaw = atan2(2*(q1q2 - q0q3), 2*(q0q0 + q1q1)-1)* 57.296f;	// ����yaw
	//Euler_Angle->yaw = atan2(2*(q1q2 - q0q3), q0q0 - q1q1 + q2q2 - q3q3)* 57.296f;	//�ҵ�
Euler_Angle->yaw = atan2(2*(q1*q2 + q0*q3), 1 - 2*q2*q2 - 2*q3*q3)* 57.296f;	// ԭ��yaw
	
	
//	for(i=0;i<9;i++)
//	{
//		*(&(DCMgb[0][0])+i) = matrix[i];
//	}

//	//ʧ�ر���(���Ե�ʱ����Թر�)
//	Safety_Check();
}

