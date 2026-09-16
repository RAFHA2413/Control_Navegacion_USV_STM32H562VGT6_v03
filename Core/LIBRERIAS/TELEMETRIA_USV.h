/*
 * TELEMETRIA_USV.h
 *
 * Libreria de telemetria para el proyecto USV.
 *
 * Funcion:
 * - Reunir los datos de los perifericos del bote.
 * - Guardarlos en la estructura USV_Telemetria.
 * - Construir la trama $PUSVD usando trama_usv.c.
 * - Enviar la trama hacia control de tierra por USART1 / XBee.
 *
 * Mantiene la filosofia de librerias usada en el proyecto del profesor.
 */

#ifndef LIBRERIAS_TELEMETRIA_USV_H_
#define LIBRERIAS_TELEMETRIA_USV_H_

#include "main.h"
#include "trama_usv.h"

#include <stddef.h>
#include <stdint.h>

/* Periodo inicial recomendado para telemetria.
 * Puede cambiarse despues si se requiere otra frecuencia.
 */
#define TELEMETRIA_USV_PERIODO_MS      500U

typedef struct
{
    UART_HandleTypeDef *huart;

    /* Datos del bote que seran enviados a tierra. */
    USV_Telemetria datos;

    /* Buffer donde se construye $PUSVD. */
    char trama_tx[USV_TRAMA_MAXIMA];

    /* Longitud de la ultima trama construida. */
    size_t longitud_tx;

    /* Control temporal. */
    uint32_t ultima_tx_ms;

    /* Diagnostico. */
    uint32_t tramas_enviadas;
    uint32_t errores_construccion;
    uint32_t errores_uart;

} TELEMETRIA_USVS;


/* Instancia principal del proyecto: USART1 / XBee. */
extern TELEMETRIA_USVS TELEMETRIA1;


/* Inicializa la estructura y valores seguros por defecto. */
void TELEMETRIA_USV_init(TELEMETRIA_USVS *TEL);


/* Actualiza los datos provenientes del GNSS. */
void TELEMETRIA_USV_GPS(
        TELEMETRIA_USVS *TEL,
        const char *utc,
        const char *latitud,
        char hemisferio_latitud,
        const char *longitud,
        char hemisferio_longitud,
        uint8_t calidad_gps,
        uint8_t satelites,
        uint16_t hdop_x10,
        int32_t altitud_x10,
        uint16_t velocidad_x10,
        uint16_t rumbo_x10);


/* Actualiza los datos provenientes de la IMU. */
void TELEMETRIA_USV_IMU(
        TELEMETRIA_USVS *TEL,
        uint16_t yaw_x10,
        int16_t pitch_x10,
        int16_t roll_x10);


/* Actualiza sensores ambientales/electricos. */
void TELEMETRIA_USV_SENSORES(
        TELEMETRIA_USVS *TEL,
        int16_t temperatura_x10,
        uint8_t inundacion,
        uint16_t voltaje_x10,
        uint16_t corriente_x10);


/* Actualiza estados generales del bote. */
void TELEMETRIA_USV_ESTADO(
        TELEMETRIA_USVS *TEL,
        uint8_t luces,
        uint8_t modo_solicitado);


/* Construye y envia inmediatamente una trama $PUSVD.
 *
 * Retorna:
 * 1 = enviada correctamente.
 * 0 = error al construir o transmitir.
 */
uint8_t TELEMETRIA_USV_Enviar(
        TELEMETRIA_USVS *TEL);


/* Tarea periodica.
 *
 * Llamar continuamente desde while(1).
 * Envia una trama cada TELEMETRIA_USV_PERIODO_MS.
 */
void TELEMETRIA_USV_Tarea(
        TELEMETRIA_USVS *TEL);


#endif /* LIBRERIAS_TELEMETRIA_USV_H_ */
