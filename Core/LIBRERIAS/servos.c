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
 * PRUEBA TEMPORAL CONTINUA DEL SERVO DE CAMARA
 *
 * Al iniciar el STM32, el servo se centra y luego conmuta
 * indefinidamente entre -90 y +90 grados.
 *
 * Con la configuracion actual de TIM3 (1 us por cuenta):
 *   -90 grados = 1000 us
 *     0 grados = 1500 us
 *   +90 grados = 2000 us
 *
 * Esta prueba es temporal. Al finalizar la validacion del servo,
 * se debe volver a SERVO_init() normal para permitir que el resto
 * del firmware continue su inicializacion.
 */
#define SERVO_PRUEBA_RETARDO_MS  1500U


//Configurar Servos Usados
SERVOS SERVO1 = {&htim3, &(TIM3->CCR3),TIM_CHANNEL_3};





void SERVO_init(SERVOS *servo)
{
    HAL_TIM_PWM_Start(servo->htim, servo->channel);

    /*
     * Primero lleva el servo al centro.
     */
    SERVO_ANG(servo, 0.0f);
    HAL_Delay(SERVO_PRUEBA_RETARDO_MS);

    /*
     * PRUEBA INFINITA:
     * -90 grados -> +90 grados -> -90 grados -> ...
     *
     * El salto completo entre ambos extremos permite verificar
     * el recorrido total del servo con pulsos de 1000 a 2000 us.
     */
    while (1)
    {
        SERVO_ANG(servo, -90.0f);
        HAL_Delay(SERVO_PRUEBA_RETARDO_MS);

        SERVO_ANG(servo, 90.0f);
        HAL_Delay(SERVO_PRUEBA_RETARDO_MS);
    }
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