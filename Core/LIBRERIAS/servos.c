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
 * Servo de camara MG996R
 * PB0 / TIM3_CH3
 *
 * Calibracion validada:
 *   -90 grados =  500 us
 *     0 grados = 1500 us
 *   +90 grados = 2500 us
 *
 * La prueba infinita fue retirada para permitir que el firmware
 * continue con IMU, Teleplot, recepcion $PUSVU y telemetria $PUSVD.
 */
SERVOS SERVO1 = {&htim3, &(TIM3->CCR3), TIM_CHANNEL_3};

void SERVO_init(SERVOS *servo)
{
    if (servo == NULL)
    {
        return;
    }

    HAL_TIM_PWM_Start(servo->htim, servo->channel);

    /* Posicion inicial segura: centro de la camara. */
    SERVO_ANG(servo, 0.0f);
}

void SERVO_ANG(SERVOS *servo, float posi)
{
    float calcu;

    if (servo == NULL)
    {
        return;
    }

    /* Limita el comando al rango fisicamente validado. */
    if (posi < ser_inf)
    {
        posi = ser_inf;
    }
    else if (posi > ser_sup)
    {
        posi = ser_sup;
    }

    calcu = (ser_lim_sup_ms - ser_lim_inf_ms) / (ser_sup - ser_inf);
    calcu = calcu * (posi - ser_inf);
    calcu = (calcu + ser_lim_inf_ms) * 1000.0f;

    *servo->ccr = (uint32_t)calcu;
}

void SERVO_MICRO(SERVOS *servo, float micro)
{
    if (servo == NULL)
    {
        return;
    }

    *servo->ccr = (uint32_t)micro;
}

void SERVO_MILI(SERVOS *servo, float milis)
{
    if (servo == NULL)
    {
        return;
    }

    *servo->ccr = (uint32_t)(milis * 1000.0f);
}

void SERVO_MUEVE(SERVOS *servo, float ini, float final, float paso, float ret)
{
    if ((servo == NULL) || (paso <= 0.0f))
    {
        return;
    }

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

    /* Asegura que la posicion final sea aplicada exactamente. */
    SERVO_ANG(servo, final);
}
