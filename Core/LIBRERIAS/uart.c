/*
 * uart.c
 *
 *  Created on: 15/11/2021
 *      Author: Alcides Ramos
 */

#include "uart.h"




void uartx_write(UART_HandleTypeDef *huart,uint8_t ch)
{
    if (huart == NULL)
    {
        return;
    }

    (void)HAL_UART_Transmit(huart, &ch, 1U, 10U);
}

void uartx_write_text(UART_HandleTypeDef *huart,char *info)
{
    size_t longitud;

    if ((huart == NULL) || (info == NULL))
    {
        return;
    }

    longitud = strlen(info);

    if (longitud == 0U)
    {
        return;
    }

    /*
     * 11.4: transmite el bloque completo en una sola llamada HAL.
     * Evita enviar caracter por caracter con timeout 0xFFFF.
     */
    (void)HAL_UART_Transmit(
        huart,
        (uint8_t *)info,
        (uint16_t)longitud,
        100U);
}


char uartx_read(UART_HandleTypeDef *huart)
{
	uint8_t dato_rx_=0;
	HAL_UART_Receive(huart,&dato_rx_, 1,HAL_MAX_DELAY);
	return(dato_rx_);
}



void uartx_read_text(UART_HandleTypeDef *huart,char  *info,const char final,char cuanto)
{
char dato_rx=0;//  los datos leidos son de 8 bits
char i=0;
    memset(info,0,cuanto);//limpia el buffer
	while(dato_rx!=final)// lee hasta que llegue el final
	{
	dato_rx =uartx_read(huart);
	*info=dato_rx;//  va almacenado en el buffer
    info++;
	i++;// incrementa contador
	if (i>cuanto-1) break;// si llegan n elementosa sale
	}
}



