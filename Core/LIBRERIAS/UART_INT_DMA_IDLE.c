/*
 * UART_INT_DMA_IDLE.c
 *
 *  Created on: Apr 9, 2025
 *      Author: ALCIDES_RAMOS
 */

#ifndef LIBRERIAS_UART_INT_DMA_IDLE_C_
#define LIBRERIAS_UART_INT_DMA_IDLE_C_

#include "main.h"
#include "UARTRX.h"

#include "GPS.h"
//incluir librerias de  interrpcines d euart usados

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
//llamar interrupciones de usart
	 uartRX_INTERRUPT(huart,Size);
   GPS_Interrupt(huart, Size);
}

/* Solicitar recuperacion fuera de la ISR; la IRQ solo captura codigo HAL. */
volatile uint32_t usv_xbee_uart1_error_irq = 0U;
volatile uint32_t usv_xbee_uart1_error_codigo = 0U;
volatile uint8_t usv_xbee_uart1_recuperar = 0U;
/* HAL_UART_ErrorCallback se define una sola vez en pcb_banco.c,
 * donde conserva USART6 e integra la notificacion USART1. */


#endif 












/* LIBRERIAS_UART_INT_DMA_IDLE_C_ */



