#ifndef CORRIENTE_LEM_H
#define CORRIENTE_LEM_H

#include "main.h"

// Sensibilidad y Vref para STM32 (ADC de 12 bits: 0 a 4095 con VREF = 3.3V)
#define LEM_VREF_VOLTS       2.50f      // Voltaje a 0 Amperios (Referencia central)
#define LEM_SENSITIVITY_V_A  0.003125f  // 3.125 mV por Amperio (625 mV / 200 A)
#define STM32_ADC_VREF       3.30f      // Voltaje de referencia del ADC de la placa
#define STM32_ADC_MAX_CODES  4095.0f    // Resolución de 12 bits

/**
 * @brief Convierte el valor crudo del ADC en corriente (Amperios).
 * @param adc_raw Valor de 16/12 bits obtenido del canal ADC (PA0 / adc1_codigo[0])
 * @return Corriente con signo en Amperios (Positivo = Carga, Negativo = Descarga)
 */
float LEM_HASS200_ObtenerCorriente(uint16_t adc_raw);

#endif /* CORRIENTE_LEM_H */