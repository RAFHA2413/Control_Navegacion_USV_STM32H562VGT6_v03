/*
 * imu_bno085_i2c.h
 *
 * Driver minimo BNO085/BNO080 por I2C para STM32 HAL.
 * Obtiene Rotation Vector y lo convierte a Roll, Pitch y Yaw.
 */

#ifndef IMU_BNO085_I2C_H
#define IMU_BNO085_I2C_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * USV 14.6: diagnostico del arranque I2C, sin lecturas adicionales del bus.
 * etapa: 0=no iniciado, 1=busqueda, 2=sin ACK, 3=fallo Product ID,
 *        4=fallo Set Feature, 5=IMU inicializada.
 * estado_4a/4b: HAL_OK=0, HAL_ERROR=1, HAL_BUSY=2, HAL_TIMEOUT=3,
 *               255=direccion no consultada.
 * error_4a/4b: ErrorCode de HAL capturado INMEDIATAMENTE tras cada prueba.
 * Nota: HAL_I2C_ERROR_TIMEOUT (32) tambien puede resultar de NACK
 * tras agotar los intentos; no demuestra por si solo un timeout fisico.
 */
typedef struct
{
    uint8_t etapa;
    uint8_t estado_4a;
    uint8_t estado_4b;
    uint32_t error_4a;
    uint32_t error_4b;
    uint32_t error_etapa;
    uint32_t isr_inicial;
    uint32_t isr_4a;
    uint32_t isr_4b;
    uint32_t estado_i2c;
    uint32_t modo_i2c;
    uint32_t isr_final;
    /* USV 14.8: 0=OK, 1=ERROR, 2=BUSY, 3=TIMEOUT; 255=sin intento. */
    uint8_t probe_hal[2][3];
    uint32_t probe_error[2][3];
    uint32_t probe_duracion_ms[2][3];
    uint32_t cr1_inicio;
    uint32_t cr2_inicio;
    uint32_t timingr_inicio;
    uint32_t addressing_mode;
} IMU_Diagnostico;

uint8_t IMU_Init(void);
void IMU_GetDiagnostico(IMU_Diagnostico *destino);
uint8_t IMU_ReadEuler(float *roll, float *pitch, float *yaw);
uint8_t IMU_GetAddress7bit(void);

#ifdef __cplusplus
}
#endif

#endif /* IMU_BNO085_I2C_H */
