#include "humedad_sensor.h"

void HUM_Init(void)
{
    // ADC y GPIO ya están configurados por CubeMX
}

uint16_t HUM_ReadRaw(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    return HAL_ADC_GetValue(&hadc1);
}

float HUM_ReadPercent(void)
{
    uint16_t raw = HUM_ReadRaw();
    return (raw / 4095.0f) * 100.0f;
}

uint8_t HUM_IsFlooded(void)
{
    return HAL_GPIO_ReadPin(HUM_DO_PORT, HUM_DO_PIN);
}
