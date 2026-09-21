#include "usv_parser.h"
#include "usv_motores.h"
#include <string.h>
#include <stdlib.h>

void PUSVU_ProcesarTrama(char *trama_rx, TIM_HandleTypeDef *htim, uint32_t canal_babor, uint32_t canal_estribor) {
    char buffer[100]; // Copia local para no alterar la trama original
    strncpy(buffer, trama_rx, sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';

    if (strstr(buffer, "$PUSVU") != NULL) {
        char *token = strtok(buffer, ","); // Extrae "$PUSVU"
        
        if (token != NULL) {
            // Extracción de datos de Babor
            token = strtok(NULL, ","); uint16_t pot_babor = token ? atoi(token) : 0;
            token = strtok(NULL, ","); uint8_t avante_bab = token ? atoi(token) : 0;
            token = strtok(NULL, ","); uint8_t atras_bab = token ? atoi(token) : 0;
            
            // Extracción de datos de Estribor
            token = strtok(NULL, ","); uint16_t pot_estrib = token ? atoi(token) : 0;
            token = strtok(NULL, ","); uint8_t avante_est = token ? atoi(token) : 0;
            token = strtok(NULL, ","); uint8_t atras_est = token ? atoi(token) : 0;

            // --- LÓGICA DE MICROSEGUNDOS (0-1000 mapeado a +/- 500us) ---
            uint16_t us_babor = PWM_NEUTRO;
            if (avante_bab == 1 && atras_bab == 0) {
                us_babor = PWM_NEUTRO + (pot_babor / 2);
            } else if (atras_bab == 1 && avante_bab == 0) {
                us_babor = PWM_NEUTRO - (pot_babor / 2);
            }

            uint16_t us_estribor = PWM_NEUTRO;
            if (avante_est == 1 && atras_est == 0) {
                us_estribor = PWM_NEUTRO + (pot_estrib / 2);
            } else if (atras_est == 1 && avante_est == 0) {
                us_estribor = PWM_NEUTRO - (pot_estrib / 2);
            }

            // --- ENVIAR ÓRDENES AL HARDWARE ---
            USV_Motor_Set(htim, canal_babor, us_babor);
            USV_Motor_Set(htim, canal_estribor, us_estribor);
        }
    }
}