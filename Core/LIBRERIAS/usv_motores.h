#ifndef USV_MOTORES_H
#define USV_MOTORES_H

#include "stm32h5xx_hal.h" // Familia correcta del microcontrolador de tu compañero

#define PWM_NEUTRO 1500
#define PWM_MAX_ADELANTE 2000
#define PWM_MAX_REVERSA 1000

// Prototipos
void USV_Motor_Calibrar(TIM_HandleTypeDef *htim, uint32_t canal_babor, uint32_t canal_estribor);
void USV_Motor_Set(TIM_HandleTypeDef *htim, uint32_t canal, uint16_t microsegundos);

#endif