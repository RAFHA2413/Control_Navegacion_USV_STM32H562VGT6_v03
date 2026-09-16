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
 * - Control de servo de camara
 * - Control de DO1-DO4 y bomba de achique
 */

#ifndef LIBRERIAS_UARTRX1_C_
#define LIBRERIAS_UARTRX1_C_

#include "UARTRX.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "servos.h"
#include "MAP_.h"

/* Libreria oficial de trama USV */
#include "trama_usv.h"

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

/*
 * Se conserva la estructura definida por el profesor.
 *
 * Se utiliza USV_TRAMA_MAXIMA = 256 bytes para que el buffer
 * sea compatible con la libreria trama_usv.
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

/*
 * Ultimo comando completo recibido desde tierra.
 */
USV_Comando comando_rx;


/*
 * Indica que se recibio correctamente al menos una trama PUSVU.
 *
 * 0 = no
 * 1 = si
 */
volatile uint8_t comando_usv_valido = 0U;


/*
 * Orden de luces.
 *
 * Se conserva hasta definir fisicamente que salida de la tarjeta
 * manejara las luces de navegacion.
 */
volatile uint8_t orden_luces = 0U;


/*
 * Valores de potencia recibidos.
 *
 * Se almacenan, pero todavia no se asignan a un PWM fisico
 * hasta confirmar los canales definitivos de propulsion.
 */
volatile uint16_t potencia_babor_x10 = 0U;
volatile uint16_t potencia_estribor_x10 = 0U;


/* ---------------------------------------------------------------
 * FUNCIONES INTERNAS
 * ---------------------------------------------------------------
 */

/**
 * @brief Lleva las salidas actualmente controladas a estado seguro.
 */
static void USV_Parada_Segura(void)
{
    /*
     * Se desactivan las cuatro salidas digitales.
     */
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


    /*
     * Bomba de achique apagada.
     */
    HAL_GPIO_WritePin(
        ACHIQUE_CTRL_GPIO_Port,
        ACHIQUE_CTRL_Pin,
        GPIO_PIN_RESET);


    /*
     * Potencia solicitada llevada a cero.
     */
    potencia_babor_x10 = 0U;
    potencia_estribor_x10 = 0U;


    /*
     * Camara centrada.
     */
    SERVO_ANG(&SERVO1, 0.0f);
}


/**
 * @brief Aplica al bote una trama PUSVU ya verificada.
 */
static void USV_Aplicar_Comando(
        const USV_Comando *comando)
{
    float angulo_recibido;

    if (comando == NULL)
    {
        return;
    }


    /*
     * STOP y FAULT tienen prioridad absoluta.
     */
    if ((comando->parada != 0U) ||
        (comando->falla_direccion != 0U))
    {
        USV_Parada_Segura();
        return;
    }


    /* -----------------------------------------------------------
     * SERVO DE CAMARA
     * -----------------------------------------------------------
     *
     * La trama transmite el angulo x10.
     *
     * Ejemplo:
     *
     * -450 = -45.0 grados
     *    0 =   0.0 grados
     * +900 = +90.0 grados
     */
    angulo_recibido =
        ((float)comando->camara_x10) / 10.0f;


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


    /* -----------------------------------------------------------
     * DIRECCION MOTORES
     * -----------------------------------------------------------
     *
     * Mapeo utilizado actualmente:
     *
     * DO1 = babor avante
     * DO2 = babor atras
     * DO3 = estribor avante
     * DO4 = estribor atras
     */

    HAL_GPIO_WritePin(
        DO1_PB12_GPIO_Port,
        DO1_PB12_Pin,
        (comando->babor_avante != 0U) ?
        GPIO_PIN_SET :
        GPIO_PIN_RESET);


    HAL_GPIO_WritePin(
        DO2_PB13_GPIO_Port,
        DO2_PB13_Pin,
        (comando->babor_atras != 0U) ?
        GPIO_PIN_SET :
        GPIO_PIN_RESET);


    HAL_GPIO_WritePin(
        DO3_PB14_GPIO_Port,
        DO3_PB14_Pin,
        (comando->estribor_avante != 0U) ?
        GPIO_PIN_SET :
        GPIO_PIN_RESET);


    HAL_GPIO_WritePin(
        DO4_PB15_GPIO_Port,
        DO4_PB15_Pin,
        (comando->estribor_atras != 0U) ?
        GPIO_PIN_SET :
        GPIO_PIN_RESET);


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
     *
     * Todavia no se asigna a un GPIO porque falta confirmar
     * fisicamente la salida destinada a luces.
     */
    orden_luces =
        comando->luces;


    /* -----------------------------------------------------------
     * POTENCIA DE PROPULSION
     * -----------------------------------------------------------
     *
     * Se conservan los valores recibidos.
     *
     * Todavia no se escriben directamente sobre TIM3_CH1 o CH4
     * hasta confirmar los canales PWM definitivos.
     */
    potencia_babor_x10 =
        comando->potencia_babor_x10;

    potencia_estribor_x10 =
        comando->potencia_estribor_x10;
}


/* ===============================================================
 * INICIALIZACION UART
 * ===============================================================
 */


/**
 * @brief Se llama una sola vez para crear el buffer de recepcion.
 *
 * @note
 * Se mantiene el nombre utilizado por el profesor.
 * En esta version se usa ReceiveToIdle por INTERRUPCION,
 * no DMA.
 */
void uartRX_it_idle_dma_init(
        UARTRXS *SERIAL)
{
    if (SERIAL == NULL)
    {
        return;
    }


    /*
     * Reserva memoria para la trama.
     */
    SERIAL->trama_rx =
        malloc(SERIAL->sizeT);


    if (SERIAL->trama_rx == NULL)
    {
        return;
    }


    /*
     * Limpia completamente el buffer.
     */
    memset(
        SERIAL->trama_rx,
        0,
        SERIAL->sizeT);


    SERIAL->flag_rx = 0;
    SERIAL->num_datos = 0;


    /*
     * Limpia errores UART.
     */
    __HAL_UART_CLEAR_OREFLAG(
        SERIAL->huart);

    __HAL_UART_FLUSH_DRREGISTER(
        SERIAL->huart);


    /*
     * Inicia recepcion hasta evento IDLE.
     *
     * IMPORTANTE:
     * Se utiliza IT porque actualmente el proyecto no tiene
     * DMA configurado para USART1.
     */
    HAL_UARTEx_ReceiveToIdle_IT(
        SERIAL->huart,
        (uint8_t *)SERIAL->trama_rx,
        SERIAL->sizeT);
}


/**
 * @brief Reinicia la recepcion UART.
 *
 * @note
 * Se conserva el nombre original por compatibilidad
 * con la libreria del profesor.
 */
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


/**
 * @brief Procesa el evento Receive To Idle de las UART.
 */
void uartRX_INTERRUPT(
        UART_HandleTypeDef *huart,
        uint16_t sizex)
{
    if (huart == NULL)
    {
        return;
    }


    /*
     * USART1 / XBee
     */
    if ((UARTRX1.flag_rx == 0) &&
        (huart->Instance ==
         UARTRX1.usart_instance))
    {
        /*
         * Evita escribir fuera del buffer.
         */
        if (sizex >= UARTRX1.sizeT)
        {
            sizex =
                UARTRX1.sizeT - 1U;
        }


        UARTRX1.num_datos =
            sizex;


        /*
         * IMPORTANTE:
         *
         * USV_LeerComando() trabaja con cadenas C.
         * Por eso la trama debe terminar en '\0'.
         */
        UARTRX1.trama_rx[sizex] =
            '\0';


        /*
         * Se avisa al main que existe una trama
         * pendiente de procesar.
         */
        UARTRX1.flag_rx = 1;
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


        /*
         * Reinicia la recepcion únicamente si
         * no existe una trama esperando procesamiento.
         */
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


/**
 * @brief Procesa la informacion recibida desde tierra.
 *
 * Permite:
 *
 * PP1:
 *    Prueba individual del servo:
 *
 *    $ANG,-45.0
 *
 * PP2:
 *    Trama definitiva de control USV:
 *
 *    $PUSVU,...*HH\r\n
 */
void procesa_rx(void)
{
    float angulo_recibido = 0.0f;


    /*
     * Verifica primero que realmente exista
     * una trama recibida.
     */
    if ((UARTRX1.trama_rx == NULL) ||
        (UARTRX1.flag_rx == 0))
    {
        return;
    }


    /* ===========================================================
     * PP1 - PRUEBA AISLADA DEL SERVO
     * ===========================================================
     *
     * Ejemplo:
     *
     * $ANG,-45.0
     */
    if (sscanf(
            UARTRX1.trama_rx,
            "$ANG,%f",
            &angulo_recibido) == 1)
    {
        /*
         * Seguridad del rango.
         */
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
    }


    /* ===========================================================
     * PP2 - TRAMA COMPLETA $PUSVU
     * ===========================================================
     */
    else if (USV_LeerComando(
                 UARTRX1.trama_rx,
                 &comando_rx) != 0U)
    {
        /*
         * La funcion USV_LeerComando ya verifico:
         *
         * - Cabecera $PUSVU
         * - cantidad de campos
         * - CRC-8
         * - rangos numericos
         * - campos digitales
         * - direccion contradictoria
         */

        comando_usv_valido = 1U;


        /*
         * Distribuye las ordenes recibidas
         * a los perifericos del bote.
         */
        USV_Aplicar_Comando(
            &comando_rx);
    }


    /*
     * La trama ya fue procesada.
     *
     * Reinicia USART1 para recibir la siguiente.
     */
    uartRX_DMA_Re_init(
        &UARTRX1);
}


#endif /* LIBRERIAS_UARTRX1_C_ */