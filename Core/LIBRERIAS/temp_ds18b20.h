#ifndef TEMP_DS18B20_H
#define TEMP_DS18B20_H

#include "main.h"

void TEMPE_Init(void);
uint8_t TEMPE_StartConversion(void);
float TEMPE_ReadResult(void);
float TEMPE_Read(void);

#endif