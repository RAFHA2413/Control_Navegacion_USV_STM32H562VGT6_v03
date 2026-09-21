#include "usv_motores.h"

void USV_Motor_Calibrar(TIM_HandleTypeDef *htim, uint32_t canal_babor, uint32_t canal_estribor) {
    // Iniciar canales PWM por hardware
    HAL_TIM_PWM_Start(htim, canal_babor);
    HAL_TIM_PWM_Start(htim, canal_estribor);

    // Paso A: Pulso máximo AVANTE
    __HAL_TIM_SET_COMPARE(htim, canal_babor, PWM_MAX_ADELANTE);
    __HAL_TIM_SET_COMPARE(htim, canal_estribor, PWM_MAX_ADELANTE);
    HAL_Delay(4000);

    // Paso B: Pulso máximo REVERSA
    __HAL_TIM_SET_COMPARE(htim, canal_babor, PWM_MAX_REVERSA);
    __HAL_TIM_SET_COMPARE(htim, canal_estribor, PWM_MAX_REVERSA);
    HAL_Delay(4000);

    // Paso C: NEUTRO y Armado final
    __HAL_TIM_SET_COMPARE(htim, canal_babor, PWM_NEUTRO);
    __HAL_TIM_SET_COMPARE(htim, canal_estribor, PWM_NEUTRO);
    HAL_Delay(3000);
}

void USV_Motor_Set(TIM_HandleTypeDef *htim, uint32_t canal, uint16_t microsegundos) {
    // Límites de seguridad
    if (microsegundos > PWM_MAX_ADELANTE) microsegundos = PWM_MAX_ADELANTE;
    if (microsegundos < PWM_MAX_REVERSA) microsegundos = PWM_MAX_REVERSA;
    
    __HAL_TIM_SET_COMPARE(htim, canal, microsegundos);
}