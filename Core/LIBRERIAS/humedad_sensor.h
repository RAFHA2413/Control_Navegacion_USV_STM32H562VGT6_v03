#ifndef HUMEDAD_SENSOR_H
#define HUMEDAD_SENSOR_H

#include "stm32f4xx_hal.h"

// Declaración externa del ADC
extern ADC_HandleTypeDef hadc1;

// Pines usados
#define HUM_DO_PORT    GPIOB
#define HUM_DO_PIN     GPIO_PIN_0

// Prototipos
void HUM_Init(void);
uint16_t HUM_ReadRaw(void);
float HUM_ReadPercent(void);
uint8_t HUM_IsFlooded(void);

#endif
