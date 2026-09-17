#include "imu_bno085_i2c.h"
#include "main.h"
#include <math.h>

extern I2C_HandleTypeDef hi2c1;

#define BNO085_ADDR (0x4A << 1)   // Dirección I2C del IMU (0x94)

// Report IDs del SH-2
#define REPORT_ID_ROTATION_VECTOR 0x05

// Buffer para recibir paquetes
uint8_t imu_buffer[32];

// ------------------ FUNCIONES INTERNAS ------------------

static void IMU_EnableRotationVector(void)
{
    uint8_t cmd[5];

    cmd[0] = 0x01;   // Set Feature Command
    cmd[1] = REPORT_ID_ROTATION_VECTOR;
    cmd[2] = 0x00;   // Time base
    cmd[3] = 0x0A;   // Interval (10 ms)
    cmd[4] = 0x00;

    HAL_I2C_Master_Transmit(&hi2c1, BNO085_ADDR, cmd, 5, 100);
}

static void quaternion_to_euler(float q1, float q2, float q3, float q0,
                                float *roll, float *pitch, float *yaw)
{
    *roll  = atan2f(2.0f*(q0*q1 + q2*q3), 1.0f - 2.0f*(q1*q1 + q2*q2)) * 57.2958f;
    *pitch = asinf(2.0f*(q0*q2 - q3*q1)) * 57.2958f;
    *yaw   = atan2f(2.0f*(q0*q3 + q1*q2), 1.0f - 2.0f*(q2*q2 + q3*q3)) * 57.2958f;
}

// ------------------ API PÚBLICA ------------------

void IMU_Init(void)
{
    HAL_Delay(50);

    IMU_EnableRotationVector();

    HAL_Delay(50);
}

void IMU_ReadEuler(float *roll, float *pitch, float *yaw)
{
    HAL_I2C_Master_Receive(&hi2c1, BNO085_ADDR, imu_buffer, 20, 100);

    uint8_t report_id = imu_buffer[0];

    if (report_id != REPORT_ID_ROTATION_VECTOR)
        return;

    int16_t q1 = (imu_buffer[5] << 8) | imu_buffer[4];
    int16_t q2 = (imu_buffer[7] << 8) | imu_buffer[6];
    int16_t q3 = (imu_buffer[9] << 8) | imu_buffer[8];
    int16_t q0 = (imu_buffer[11] << 8) | imu_buffer[10];

    float scale = 1.0f / (1 << 14);

    float fq1 = q1 * scale;
    float fq2 = q2 * scale;
    float fq3 = q3 * scale;
    float fq0 = q0 * scale;

    quaternion_to_euler(fq1, fq2, fq3, fq0, roll, pitch, yaw);
}
