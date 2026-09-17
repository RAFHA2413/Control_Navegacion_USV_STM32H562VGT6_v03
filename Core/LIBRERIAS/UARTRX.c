/*
 * UARTRX1.c
 *
 * Created on: Apr 9, 2025
 * Author: ALCIDES_RAMOS
 *
 * Adaptada para proyecto USV:
 * - Recepcion USART1 / XBee
 * - Trama corta de prueba $ANG,xx.x
 * - Trama completa $PUSVU
 * - Decodificacion mediante trama_usv
 * - Distribucion de ordenes a los perifericos del bote
 */

#ifndef LIBRERIAS_UARTRX1_C_
#define LIBRERIAS_UARTRX1_C_

#include "UARTRX.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "servos.h"
#include "trama_usv.h"
#include "TELEMETRIA_USV.h"

/*
 * PRUEBA DE INTEGRACION ESTACION -> BOTE -> SERVO.
 *
 * 1 = una trama $PUSVU valida aplica UNICAMENTE el campo camara al MG996R.
 *     Bomba, luces y propulsion se ignoran durante esta prueba.
 *     Esto permite validar el joystick de tierra y el servo sin que otros
 *     perifericos puedan interferir.
 *
 * 0 = funcionamiento completo normal del comando $PUSVU.
 */
#define PRUEBA_INTEGRACION_CAMARA_TIERRA  1


/* ---------------------------------------------------------------
 * VARIABLES EXTERNAS
 * ---------------------------------------------------------------
 */

extern SERVOS SERVO1;

/* USART usado para comunicacion XBee / control de tierra */
extern UART_HandleTypeDef huart1;


/* ---------------------------------------------------------------
 * CONFIGURACION DE UART RX
 * ---------------------------------------------------------------
 */

UARTRXS UARTRX1 =
{
    &huart1,
    USART1,
    USV_TRAMA_MAXIMA
};


/* ---------------------------------------------------------------
 * VARIABLES DEL PROYECTO USV
 * ---------------------------------------------------------------
 */

/* Ultimo comando completo recibido desde tierra. */
USV_Comando comando_rx;

/* 1 cuando se recibio correctamente al menos una trama $PUSVU. */
volatile uint8_t comando_usv_valido = 0U;

/* Diagnostico temporal ESTACION -> BOTE. */
volatile uint32_t usv_rx_eventos = 0U;
volatile uint32_t usv_tramas_validas = 0U;
volatile int16_t usv_camara_recibida = 0;

/* Se conserva hasta asignar fisicamente la salida de luces. */
volatile uint8_t orden_luces = 0U;

/*
 * Potencias finales recibidas desde tierra, en porcentaje entero 0..100.
 * PB4/TIM3_CH1 corresponde a babor y PB1/TIM3_CH4 a estribor, pero todavia
 * no se escribe PWM fisico hasta confirmar el tipo de señal requerido por
 * los controladores de propulsion.
 */
volatile uint16_t potencia_babor = 0U;
volatile uint16_t potencia_estribor = 0U;


/* ---------------------------------------------------------------
 * FUNCIONES INTERNAS
 * ---------------------------------------------------------------
 */

/**
 * @brief Lleva solamente la propulsion a estado seguro.
 *
 * La trama vigente indica que la parada tiene prioridad sobre la propulsion.
 * La camara, la bomba y las luces permanecen disponibles.
 */
static void USV_Parada_Propulsion(void)
{
    HAL_GPIO_WritePin(
        DO1_PB12_GPIO_Port,
        DO1_PB12_Pin,
        GPIO_PIN_RESET);

    HAL_GPIO_WritePin(
        DO2_PB13_GPIO_Port,
        DO2_PB13_Pin,
        GPIO_PIN_RESET);

    HAL_GPIO_WritePin(
        DO3_PB14_GPIO_Port,
        DO3_PB14_Pin,
        GPIO_PIN_RESET);

    HAL_GPIO_WritePin(
        DO4_PB15_GPIO_Port,
        DO4_PB15_Pin,
        GPIO_PIN_RESET);

    potencia_babor = 0U;
    potencia_estribor = 0U;
}


/**
 * @brief Aplica al bote una trama $PUSVU ya verificada.
 */
static void USV_Aplicar_Comando(
        const USV_Comando *comando)
{
    float angulo_recibido;
    uint8_t modo_telemetria;

    if (comando == NULL)
    {
        return;
    }

    /* -----------------------------------------------------------
     * SERVO DE CAMARA
     * -----------------------------------------------------------
     * La trama vigente entrega grados enteros directamente:
     * -90 .. 0 .. +90.
     */
    angulo_recibido =
        (float)comando->camara;

    if (angulo_recibido < -90.0f)
    {
        angulo_recibido = -90.0f;
    }

    if (angulo_recibido > 90.0f)
    {
        angulo_recibido = 90.0f;
    }

    SERVO_ANG(
        &SERVO1,
        angulo_recibido);

#if PRUEBA_INTEGRACION_CAMARA_TIERRA
    /*
     * Para esta prueba no se ejecuta ninguna otra orden recibida desde tierra.
     * Solo se valida: joystick -> UART -> $PUSVU -> servo de camara.
     */
    return;
#endif


    /* -----------------------------------------------------------
     * BOMBA DE ACHIQUE
     * -----------------------------------------------------------
     */
    HAL_GPIO_WritePin(
        ACHIQUE_CTRL_GPIO_Port,
        ACHIQUE_CTRL_Pin,
        (comando->bomba != 0U) ?
        GPIO_PIN_SET :
        GPIO_PIN_RESET);


    /* -----------------------------------------------------------
     * LUCES
     * -----------------------------------------------------------
     * Todavia no existe GPIO definitivo para luces.
     */
    orden_luces =
        comando->luces;


    /* -----------------------------------------------------------
     * ESTADO PARA TELEMETRIA
     * -----------------------------------------------------------
     * Trama $PUSVD:
     * 0 = reposo, 1 = remoto, 2 = reconectar.
     */
    if (comando->reconexion != 0U)
    {
        modo_telemetria = 2U;
    }
    else
    {
        modo_telemetria = 1U;
    }

    TELEMETRIA_USV_ESTADO(
        &TELEMETRIA1,
        orden_luces,
        modo_telemetria);


    /* -----------------------------------------------------------
     * SEGURIDAD DE PROPULSION
     * -----------------------------------------------------------
     * STOP y FAULT tienen prioridad sobre las salidas de movimiento.
     * La camara, bomba y luces ya fueron atendidas arriba.
     */
    if ((comando->parada != 0U) ||
        (comando->falla_direccion != 0U))
    {
        USV_Parada_Propulsion();
        return;
    }


    /* -----------------------------------------------------------
     * DIRECCION MOTORES
     * -----------------------------------------------------------
     * DO1 = babor avante
     * DO2 = babor atras
     * DO3 = estribor avante
     * DO4 = estribor atras
     */
    HAL_GPIO_WritePin(
        DO1_PB12_GPIO_Port,
        DO1_PB12_Pin,
        (comando->babor_avante != 0U) ?
        GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(
        DO2_PB13_GPIO_Port,
        DO2_PB13_Pin,
        (comando->babor_atras != 0U) ?
        GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(
        DO3_PB14_GPIO_Port,
        DO3_PB14_Pin,
        (comando->estribor_avante != 0U) ?
        GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(
        DO4_PB15_GPIO_Port,
        DO4_PB15_Pin,
        (comando->estribor_atras != 0U) ?
        GPIO_PIN_SET : GPIO_PIN_RESET);


    /* -----------------------------------------------------------
     * POTENCIA DE PROPULSION
     * -----------------------------------------------------------
     * La estacion de tierra ya hizo la mezcla. El bote NO vuelve
     * a calcularla usando potencia_global o direccion.
     *
     * Se guardan porcentajes finales 0..100. La salida PWM fisica
     * se habilitara cuando se confirme el tipo de señal del driver/ESC.
     */
    potencia_babor =
        comando->potencia_babor;

    potencia_estribor =
        comando->potencia_estribor;
}


/* ===============================================================
 * INICIALIZACION UART
 * ===============================================================
 */

void uartRX_it_idle_dma_init(
        UARTRXS *SERIAL)
{
    if (SERIAL == NULL)
    {
        return;
    }

    SERIAL->trama_rx =
        malloc(SERIAL->sizeT);

    if (SERIAL->trama_rx == NULL)
    {
        return;
    }

    memset(
        SERIAL->trama_rx,
        0,
        SERIAL->sizeT);

    SERIAL->flag_rx = 0;
    SERIAL->num_datos = 0;

    __HAL_UART_CLEAR_OREFLAG(
        SERIAL->huart);

    __HAL_UART_FLUSH_DRREGISTER(
        SERIAL->huart);

    HAL_UARTEx_ReceiveToIdle_IT(
        SERIAL->huart,
        (uint8_t *)SERIAL->trama_rx,
        SERIAL->sizeT);
}


void uartRX_DMA_Re_init(
        UARTRXS *SERIAL)
{
    if ((SERIAL == NULL) ||
        (SERIAL->trama_rx == NULL))
    {
        return;
    }

    memset(
        SERIAL->trama_rx,
        0,
        SERIAL->sizeT);

    SERIAL->flag_rx = 0;
    SERIAL->num_datos = 0;

    __HAL_UART_CLEAR_OREFLAG(
        SERIAL->huart);

    __HAL_UART_FLUSH_DRREGISTER(
        SERIAL->huart);

    HAL_UARTEx_ReceiveToIdle_IT(
        SERIAL->huart,
        (uint8_t *)SERIAL->trama_rx,
        SERIAL->sizeT);
}


/* ===============================================================
 * INTERRUPCION DE RECEPCION
 * ===============================================================
 */

void uartRX_INTERRUPT(
        UART_HandleTypeDef *huart,
        uint16_t sizex)
{
    if (huart == NULL)
    {
        return;
    }

    if ((UARTRX1.flag_rx == 0) &&
        (huart->Instance == UARTRX1.usart_instance))
    {
        if (sizex >= UARTRX1.sizeT)
        {
            sizex =
                UARTRX1.sizeT - 1U;
        }

        UARTRX1.num_datos =
            sizex;

        UARTRX1.trama_rx[sizex] =
            '\0';

        UARTRX1.flag_rx = 1;
        usv_rx_eventos++;
    }
}


/* ===============================================================
 * MANEJO DE ERRORES UART
 * ===============================================================
 */

void uartRX_Errores(
        UART_HandleTypeDef *huart)
{
    if (huart == NULL)
    {
        return;
    }

    if (huart->Instance ==
        UARTRX1.usart_instance)
    {
        __HAL_UART_CLEAR_OREFLAG(
            UARTRX1.huart);

        __HAL_UART_FLUSH_DRREGISTER(
            UARTRX1.huart);

        if (UARTRX1.flag_rx == 0)
        {
            HAL_UART_AbortReceive(
                UARTRX1.huart);

            HAL_UARTEx_ReceiveToIdle_IT(
                UARTRX1.huart,
                (uint8_t *)UARTRX1.trama_rx,
                UARTRX1.sizeT);
        }
    }
}


/* ===============================================================
 * PROCESAMIENTO DE TRAMA
 * ===============================================================
 */

void procesa_rx(void)
{
    float angulo_recibido = 0.0f;

    if ((UARTRX1.trama_rx == NULL) ||
        (UARTRX1.flag_rx == 0))
    {
        return;
    }

    /* PP1 - prueba aislada del servo: $ANG,-45.0 */
    if (sscanf(
            (char *)UARTRX1.trama_rx,
            "$ANG,%f",
            &angulo_recibido) == 1)
    {
        if (angulo_recibido < -90.0f)
        {
            angulo_recibido = -90.0f;
        }

        if (angulo_recibido > 90.0f)
        {
            angulo_recibido = 90.0f;
        }

#if (SERVO_PRUEBA_LOCAL_BOTE == 0)
        SERVO_ANG(
            &SERVO1,
            angulo_recibido);
#endif
    }

    /* PP2 - trama completa oficial $PUSVU,...*HH\r\n */
    else if (USV_LeerComando(
                 (const char *)UARTRX1.trama_rx,
                 &comando_rx) != 0U)
    {
        comando_usv_valido = 1U;
        usv_tramas_validas++;
        usv_camara_recibida = comando_rx.camara;

        USV_Aplicar_Comando(
            &comando_rx);
    }
}


#endif /* LIBRERIAS_UARTRX1_C_ */
