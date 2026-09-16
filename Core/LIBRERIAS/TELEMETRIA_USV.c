/*
 * TELEMETRIA_USV.c
 *
 * Libreria de telemetria para el proyecto USV.
 *
 * Usa la trama oficial:
 *
 * $PUSVD,...*HH\r\n
 *
 * El formato, rangos y CRC son gestionados por trama_usv.c.
 */

#include "TELEMETRIA_USV.h"

#include <string.h>


/* USART utilizado por el XBee. */
extern UART_HandleTypeDef huart1;


/* Instancia principal de telemetria. */
TELEMETRIA_USVS TELEMETRIA1 =
{
    .huart = &huart1
};


/* ================================================================
 * FUNCIONES INTERNAS
 * ================================================================
 */


/* Copia una cadena asegurando siempre terminacion '\0'. */
static void TELEMETRIA_USV_CopiarTexto(
        char *destino,
        size_t capacidad,
        const char *origen)
{
    size_t longitud;

    if ((destino == NULL) || (capacidad == 0U))
    {
        return;
    }

    destino[0] = '\0';

    if (origen == NULL)
    {
        return;
    }

    longitud = strlen(origen);

    if (longitud >= capacidad)
    {
        longitud = capacidad - 1U;
    }

    memcpy(destino, origen, longitud);
    destino[longitud] = '\0';
}


/* ================================================================
 * INICIALIZACION
 * ================================================================
 */


void TELEMETRIA_USV_init(
        TELEMETRIA_USVS *TEL)
{
    UART_HandleTypeDef *uart_guardada;

    if (TEL == NULL)
    {
        return;
    }

    /*
     * Conserva la UART antes de limpiar la estructura.
     */
    uart_guardada = TEL->huart;

    memset(TEL, 0, sizeof(*TEL));

    TEL->huart = uart_guardada;


    /*
     * Valores iniciales validos para que la trama pueda construirse
     * incluso antes de recibir datos reales de los sensores.
     *
     * GPS sin solucion:
     * calidad = 0
     * satelites = 0
     * coordenadas = 0
     * HDOP = 50.0
     */
    TELEMETRIA_USV_CopiarTexto(
        TEL->datos.utc,
        sizeof(TEL->datos.utc),
        "000000.00");

    TELEMETRIA_USV_CopiarTexto(
        TEL->datos.latitud,
        sizeof(TEL->datos.latitud),
        "0000.0000");

    TEL->datos.hemisferio_latitud = 'N';

    TELEMETRIA_USV_CopiarTexto(
        TEL->datos.longitud,
        sizeof(TEL->datos.longitud),
        "00000.0000");

    TEL->datos.hemisferio_longitud = 'E';

    TEL->datos.calidad_gps = 0U;
    TEL->datos.satelites = 0U;

    /*
     * 500 representa HDOP = 50.0.
     * Es un valor valido dentro de la trama y representa
     * una condicion de precision muy pobre / sin uso practico.
     */
    TEL->datos.hdop_x10 = 500U;

    TEL->datos.altitud_x10 = 0;
    TEL->datos.velocidad_x10 = 0U;
    TEL->datos.rumbo_x10 = 0U;

    TEL->datos.yaw_x10 = 0U;
    TEL->datos.pitch_x10 = 0;
    TEL->datos.roll_x10 = 0;

    TEL->datos.temperatura_x10 = 0;
    TEL->datos.inundacion = 0U;
    TEL->datos.voltaje_x10 = 0U;
    TEL->datos.corriente_x10 = 0U;

    TEL->datos.luces = 0U;
    TEL->datos.modo_solicitado = 0U;

    TEL->ultima_tx_ms = HAL_GetTick();
}


/* ================================================================
 * ACTUALIZACION DE DATOS
 * ================================================================
 */


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
        uint16_t rumbo_x10)
{
    if (TEL == NULL)
    {
        return;
    }

    TELEMETRIA_USV_CopiarTexto(
        TEL->datos.utc,
        sizeof(TEL->datos.utc),
        utc);

    TELEMETRIA_USV_CopiarTexto(
        TEL->datos.latitud,
        sizeof(TEL->datos.latitud),
        latitud);

    TEL->datos.hemisferio_latitud =
        hemisferio_latitud;

    TELEMETRIA_USV_CopiarTexto(
        TEL->datos.longitud,
        sizeof(TEL->datos.longitud),
        longitud);

    TEL->datos.hemisferio_longitud =
        hemisferio_longitud;

    TEL->datos.calidad_gps =
        calidad_gps;

    TEL->datos.satelites =
        satelites;

    TEL->datos.hdop_x10 =
        hdop_x10;

    TEL->datos.altitud_x10 =
        altitud_x10;

    TEL->datos.velocidad_x10 =
        velocidad_x10;

    TEL->datos.rumbo_x10 =
        rumbo_x10;
}


void TELEMETRIA_USV_IMU(
        TELEMETRIA_USVS *TEL,
        uint16_t yaw_x10,
        int16_t pitch_x10,
        int16_t roll_x10)
{
    if (TEL == NULL)
    {
        return;
    }

    TEL->datos.yaw_x10 =
        yaw_x10;

    TEL->datos.pitch_x10 =
        pitch_x10;

    TEL->datos.roll_x10 =
        roll_x10;
}


void TELEMETRIA_USV_SENSORES(
        TELEMETRIA_USVS *TEL,
        int16_t temperatura_x10,
        uint8_t inundacion,
        uint16_t voltaje_x10,
        uint16_t corriente_x10)
{
    if (TEL == NULL)
    {
        return;
    }

    TEL->datos.temperatura_x10 =
        temperatura_x10;

    TEL->datos.inundacion =
        inundacion;

    TEL->datos.voltaje_x10 =
        voltaje_x10;

    TEL->datos.corriente_x10 =
        corriente_x10;
}


void TELEMETRIA_USV_ESTADO(
        TELEMETRIA_USVS *TEL,
        uint8_t luces,
        uint8_t modo_solicitado)
{
    if (TEL == NULL)
    {
        return;
    }

    TEL->datos.luces =
        luces;

    TEL->datos.modo_solicitado =
        modo_solicitado;
}


/* ================================================================
 * CONSTRUCCION Y ENVIO
 * ================================================================
 */


uint8_t TELEMETRIA_USV_Enviar(
        TELEMETRIA_USVS *TEL)
{
    HAL_StatusTypeDef estado;

    if ((TEL == NULL) ||
        (TEL->huart == NULL))
    {
        return 0U;
    }


    /*
     * Construye la trama completa y calcula automaticamente el CRC.
     */
    TEL->longitud_tx =
        USV_ConstruirTelemetria(
            TEL->trama_tx,
            sizeof(TEL->trama_tx),
            &TEL->datos);


    if (TEL->longitud_tx == 0U)
    {
        TEL->errores_construccion++;
        return 0U;
    }


    /*
     * Primera version:
     * transmision bloqueante corta por USART1.
     *
     * Esto facilita la validacion inicial del enlace XBee.
     * Mas adelante puede migrarse a IT/DMA si se requiere.
     */
    estado =
        HAL_UART_Transmit(
            TEL->huart,
            (uint8_t *)TEL->trama_tx,
            (uint16_t)TEL->longitud_tx,
            100U);


    if (estado != HAL_OK)
    {
        TEL->errores_uart++;
        return 0U;
    }


    TEL->tramas_enviadas++;

    return 1U;
}


/* ================================================================
 * TAREA PERIODICA
 * ================================================================
 */


void TELEMETRIA_USV_Tarea(
        TELEMETRIA_USVS *TEL)
{
    uint32_t ahora;

    if (TEL == NULL)
    {
        return;
    }

    ahora = HAL_GetTick();

    if ((uint32_t)(ahora - TEL->ultima_tx_ms) >=
        TELEMETRIA_USV_PERIODO_MS)
    {
        TEL->ultima_tx_ms = ahora;

        (void)TELEMETRIA_USV_Enviar(TEL);
    }
}
