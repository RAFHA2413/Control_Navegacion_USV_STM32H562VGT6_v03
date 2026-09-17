/*
 * servos.c
 *
 *  Created on: Apr 8, 2023
 *      Author: Alcides Ramos
 */


#include "servos.h"


//incluir los timer usados
extern TIM_HandleTypeDef htim3;


/*
 * PRUEBA TEMPORAL DEL SERVO DE CAMARA
 *
 * 1 = al reiniciar el STM32 ejecuta automaticamente:
 *     0 -> +90 -> 0 -> -90 -> 0 grados.
 *
 * 0 = funcionamiento normal sin prueba automatica.
 *
 * Cuando terminemos de validar el servo, cambiar a 0.
 */
#define SERVO_PRUEBA_AUTOMATICA  1
#define SERVO_PRUEBA_RETARDO_MS  1500U


//Configurar Servos Usados
SERVOS SERVO1 = {&htim3, &(TIM3->CCR3),TIM_CHANNEL_3};





void SERVO_init(SERVOS *servo)
{
    HAL_TIM_PWM_Start(servo->htim, servo->channel);

#if SERVO_PRUEBA_AUTOMATICA
    /*
     * Secuencia aislada para verificar:
     * TIM3_CH3 -> PB0 -> señal del servo de camara.
     *
     * Con TIM3 a 1 us por cuenta se esperan aproximadamente:
     *   0 grados   = 1500 us
     *  +90 grados  = 2000 us
     *  -90 grados  = 1000 us
     */
    SERVO_ANG(servo, 0.0f);
    HAL_Delay(SERVO_PRUEBA_RETARDO_MS);

    SERVO_ANG(servo, 90.0f);
    HAL_Delay(SERVO_PRUEBA_RETARDO_MS);

    SERVO_ANG(servo, 0.0f);
    HAL_Delay(SERVO_PRUEBA_RETARDO_MS);

    SERVO_ANG(servo, -90.0f);
    HAL_Delay(SERVO_PRUEBA_RETARDO_MS);

    SERVO_ANG(servo, 0.0f);
    HAL_Delay(SERVO_PRUEBA_RETARDO_MS);
#endif
}


void SERVO_ANG(SERVOS *servo,float posi)

{
     float calcu;
      calcu=(ser_lim_sup_ms - ser_lim_inf_ms) / (ser_sup - ser_inf);
      calcu=calcu*(posi-ser_inf);
      calcu=(calcu+ser_lim_inf_ms)*1000.0;// para pasarlo a microsegundo
      *servo->ccr = calcu;

}


void SERVO_MICRO(SERVOS *servo,float micro)

{
     *servo->ccr = micro;
}


void SERVO_MILI(SERVOS *servo,float milis)

{
     float calcu;
      calcu=milis*1000.0;// para pasarlo a microsegundo
      *servo->ccr = calcu;
}


void SERVO_MUEVE(SERVOS *servo,float ini, float final,float paso,float ret)
{
if (final>ini)
{               //-90    90
  for   (float c=ini; c<final;c+=paso)
  {
    SERVO_ANG(servo, c);
    HAL_Delay(ret) ;
  }

}
else
{                  //90     -90
      for   (float c=ini; c>final;c-=paso)
      {
        SERVO_ANG(servo, c);
        HAL_Delay(ret) ;
      }

}
}