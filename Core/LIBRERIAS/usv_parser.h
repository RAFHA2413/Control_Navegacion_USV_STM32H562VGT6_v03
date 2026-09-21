#ifndef USV_PARSER_H
#define USV_PARSER_H

#include "stm32h5xx_hal.h"

// Prototipo de la función que lee la trama y mueve los motores automáticamente
void PUSVU_ProcesarTrama(char *trama_rx, TIM_HandleTypeDef *htim, uint32_t canal_babor, uint32_t canal_estribor);

#endif