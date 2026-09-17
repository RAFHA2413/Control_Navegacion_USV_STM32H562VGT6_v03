#ifndef IMU_BNO085_I2C_H
#define IMU_BNO085_I2C_H

#include "main.h"

void IMU_Init(void);
void IMU_ReadEuler(float *roll, float *pitch, float *yaw);

#endif
