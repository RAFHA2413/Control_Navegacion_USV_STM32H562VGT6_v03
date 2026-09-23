#ifndef BATERIA_MOTORES_H
#define BATERIA_MOTORES_H

#include "main.h"

/* 
 * @brief Calcula el voltaje real de la batería y lo envía a la pantalla Nextion.
 * @param huart: Puntero al periférico UART conectado a la Nextion.
 * @param adc_raw: Valor crudo (0-4095) leído por el ADC mediante DMA.
 */
void BATERIA_Actualizar_Nextion(UART_HandleTypeDef *huart, uint16_t adc_raw);

#endif /* BATERIA_NEXTION_H */