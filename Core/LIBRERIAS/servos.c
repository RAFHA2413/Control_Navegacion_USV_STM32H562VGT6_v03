/*
 * servos.c
 *
 *  Created on: Apr 8, 2023
 *      Author: Alcides Ramos
 */

#include "servos.h"

/* incluir los timer usados */
extern TIM_HandleTypeDef htim3;

/*
 * PRUEBA TEMPORAL CONTINUA DEL SERVO DE CAMARA MG996R
 *
 * El servo probado recorrio aproximadamente +/-45 grados con
 * pulsos de 1000 a 2000 us. Para validar el recorrido completo
 * del MG996R se prueba ahora el rango ampliado 500 a 2500 us.
 *
 * Con la configuracion actual de TIM3 (1 us por cuenta):
 *    extremo 1 =  500 us
 *    centro    = 1500 us
 *    extremo 2 = 2500 us
 *
 * La prueba es intencionalmente infinita y bloquea el resto del
 * firmware mientras se valida exclusivamente el servo.
 */
#define SERVO_PRUEBA_RETARDO_MS  1500U
#define SERVO_PULSO_MIN_US       500.0f
#define SERVO_PULSO_CENTRO_US    1500.0f
#define SERVO_PULSO_MAX_US       2500.0f

/* Configurar Servos Usados */
SERVOS SERVO1 = {&htim3, &(TIM3->CCR3), TIM_CHANNEL_3};

void SERVO_init(SERVOS *servo)
{
    HAL_TIM_PWM_Start(servo->htim, servo->channel);

    /* Centro antes de iniciar la prueba de extremos. */
    SERVO_MICRO(servo, SERVO_PULSO_CENTRO_US);
    HAL_Delay(SERVO_PRUEBA_RETARDO_MS);

    /*
     * PRUEBA INFINITA DEL RECORRIDO COMPLETO:
     * 500 us -> 2500 us -> 500 us -> 2500 us -> ...
     */
    while (1)
    {
        SERVO_MICRO(servo, SERVO_PULSO_MIN_US);
        HAL_Delay(SERVO_PRUEBA_RETARDO_MS);

        SERVO_MICRO(servo, SERVO_PULSO_MAX_US);
        HAL_Delay(SERVO_PRUEBA_RETARDO_MS);
    }
}

void SERVO_ANG(SERVOS *servo, float posi)
{
    float calcu;
    calcu = (ser_lim_sup_ms - ser_lim_inf_ms) / (ser_sup - ser_inf);
    calcu = calcu * (posi - ser_inf);
    calcu = (calcu + ser_lim_inf_ms) * 1000.0f;
    *servo->ccr = (uint32_t)calcu;
}

void SERVO_MICRO(SERVOS *servo, float micro)
{
    *servo->ccr = (uint32_t)micro;
}

void SERVO_MILI(SERVOS *servo, float milis)
{
    float calcu;
    calcu = milis * 1000.0f;
    *servo->ccr = (uint32_t)calcu;
}

void SERVO_MUEVE(SERVOS *servo, float ini, float final, float paso, float ret)
{
    if (final > ini)
    {
        for (float c = ini; c < final; c += paso)
        {
            SERVO_ANG(servo, c);
            HAL_Delay((uint32_t)ret);
        }
    }
    else
    {
        for (float c = ini; c > final; c -= paso)
        {
            SERVO_ANG(servo, c);
            HAL_Delay((uint32_t)ret);
        }
    }
}
