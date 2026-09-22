#include "corriente_lem.h"

float LEM_HASS200_ObtenerCorriente(uint16_t adc_raw)
{
    // 1. Convierte el código crudo del ADC a Voltios
    float v_medido = ((float)adc_raw / STM32_ADC_MAX_CODES) * STM32_ADC_VREF;
    
    // 2. Calcula la corriente: (Vmedido - Vref) / Sensibilidad
    float corriente_amps = (v_medido - LEM_VREF_VOLTS) / LEM_SENSITIVITY_V_A;
    
    return corriente_amps;
}