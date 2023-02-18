#ifndef _MPU_DMP_API_H
#define _MPU_DMP_API_H

#include "stm32f10x.h"

void SetReportFlag(u8 rawDataReportCmd, u8 quatDataReportCmd);
void gyro_data_ready_cb(void);
u8 mpu_dmp_init(void);
u8 mpu_dmp_getdata(void);


void MPU_INT_Config(void);

#endif
