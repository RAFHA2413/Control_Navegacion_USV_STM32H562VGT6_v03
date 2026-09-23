#include "bateria_motores.h"
#include <stdio.h>

// Constantes físicas del sistema
#define FACTOR_DIVISOR  5.255f // R1=20k, R2=4.7k
#define VREF_STM        3.3f
#define ADC_MAX_VAL     4095.0f

void BATERIA_Actualizar_Nextion(UART_HandleTypeDef *huart, uint16_t adc_raw)
{
    // 1. Calcular el voltaje de la batería
    float voltage_stm = ((float)adc_raw * VREF_STM) / ADC_MAX_VAL;
    float voltage_bateria = voltage_stm * FACTOR_DIVISOR;

    // 2. Extraer parte entera y decimal
    int parte_entera = (int)voltage_bateria;
    int parte_decimal = (int)((voltage_bateria - parte_entera) * 100);

    // 3. Formatear el comando para la pantalla Nextion
    char buffer_uart[50];
    int len = snprintf(buffer_uart, sizeof(buffer_uart), "tVoltaje.txt=\"%d.%02d V\"", parte_entera, parte_decimal);

    // 4. Transmitir comando
    if (len > 0)
    {
        HAL_UART_Transmit(huart, (uint8_t*)buffer_uart, (uint16_t)len, 100);
        
        // 5. Transmitir los 3 bytes de finalización de Nextion
        uint8_t nextion_end[3] = {0xFF, 0xFF, 0xFF};
        HAL_UART_Transmit(huart, nextion_end, 3, 100);
    }
}