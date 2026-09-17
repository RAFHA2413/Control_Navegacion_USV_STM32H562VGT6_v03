/*
 * servos.h
 *
 *  Created on: Apr 8, 2023
 *      Author: Alcides Ramos
 */

#ifndef LIBRERIAS_SERVOS_H_
#define LIBRERIAS_SERVOS_H_

#include "main.h"

/*
 * Calibracion validada para el servo de camara MG996R.
 *
 * Con TIM3 configurado a 1 us por cuenta:
 *   -90 grados =  500 us
 *     0 grados = 1500 us
 *   +90 grados = 2500 us
 *
 * Estos limites fueron comprobados fisicamente en el proyecto.
 */
#define ser_lim_inf_ms    0.5f
#define ser_lim_sup_ms    2.5f

/* Movimiento deseado del servo */
#define ser_inf          -90.0f
#define ser_sup           90.0f

typedef struct {
    TIM_HandleTypeDef *htim;
    volatile uint32_t *ccr;
    uint32_t channel;
} SERVOS;

/* Servo de camara: PB0 / TIM3_CH3 */
extern SERVOS SERVO1;

void SERVO_init(SERVOS *servo);
void SERVO_ANG(SERVOS *servo, float posi);
void SERVO_MICRO(SERVOS *servo, float micros);
void SERVO_MILI(SERVOS *servo, float milis);
void SERVO_MUEVE(SERVOS *servo, float ini, float final, float paso, float ret);

#endif /* LIBRERIAS_SERVOS_H_ */
