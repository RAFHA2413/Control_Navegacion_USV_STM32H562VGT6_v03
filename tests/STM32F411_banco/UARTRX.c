/* F411: responder de PRUEBA XBee AT; los valores de telemetria son sinteticos. */
#include "UARTRX.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern UART_HandleTypeDef huart1;
UARTRXS UARTRX1 = {&huart1, USART1, 500};
static uint8_t rx_mem[501];
static char linea[256];
static uint16_t usados;
static uint8_t crc8(const uint8_t *p, size_t n) {
    uint8_t c=0U, k; size_t i;
    for(i=0U;i<n;++i) {c^=p[i]; for(k=0U;k<8U;++k)
        c=(uint8_t)((c<<1U)^((c&0x80U)?0x07U:0U));}
    return c;
}
void uartRX_it_idle_dma_init(UARTRXS *s) {
    s->trama_rx=rx_mem; memset(rx_mem,0,sizeof(rx_mem));s->flag_rx=0;
    if(HAL_UARTEx_ReceiveToIdle_DMA(s->huart,s->trama_rx,s->sizeT)!=HAL_OK)
        Error_Handler();
    __HAL_DMA_DISABLE_IT(s->huart->hdmarx,DMA_IT_HT);
}
void uartRX_DMA_Re_init(UARTRXS *s) {
    memset(rx_mem,0,sizeof(rx_mem));s->flag_rx=0;
    if(HAL_UARTEx_ReceiveToIdle_DMA(s->huart,s->trama_rx,s->sizeT)!=HAL_OK)
        Error_Handler();
    __HAL_DMA_DISABLE_IT(s->huart->hdmarx,DMA_IT_HT);
}
void uartRX_INTERRUPT(UART_HandleTypeDef *h,uint16_t size) {
    if(h->Instance==USART1 && UARTRX1.flag_rx==0) {
        HAL_UART_DMAStop(h);
        if(size<=UARTRX1.sizeT)UARTRX1.trama_rx[size]=0U;
        UARTRX1.num_datos=size;UARTRX1.flag_rx=1;
    }
}
void uartRX_Errores(UART_HandleTypeDef *h) {
    if(h->Instance==USART1) {HAL_UART_DMAStop(h);uartRX_DMA_Re_init(&UARTRX1);}
}
static uint8_t comando_valido(const char *s) {
    int b,e,c,bomba,parada,n=-1;
    int r=sscanf(s,"$PUSVU,%d,%d,%d,%d,%d\r\n%n",&b,&e,&c,&bomba,&parada,&n);
    return (uint8_t)(r==5 && n==(int)strlen(s) &&
        b>=-100 && b<=100 && e>=-100 && e<=100 &&
        c>=-90 && c<=90 && (bomba==0||bomba==1) &&
        (parada==0||parada==1));
}
static void responder(void) {
    static const char body[]=
      "$PUSVD,102831.00,1024.7404,N,07531.9268,W,1,08,1.2,5.0,M,"
      "12.5,N,185.4,T,185.0,T,+1.2,P,-0.5,R,24.5,C,1,12.4,V,2.3,C,1,STATE,1";
    char frame[180];
    int n=snprintf(frame,sizeof(frame),"%s*%02X\r\n",body,
        crc8((const uint8_t*)body+1U,strlen(body)-1U));
    if(n>0 && n<(int)sizeof(frame))
        (void)HAL_UART_Transmit(&huart1,(uint8_t*)frame,(uint16_t)n,200U);
}
void procesa_rx(void) {
    uint16_t i;
    if(UARTRX1.trama_rx==NULL || UARTRX1.num_datos>UARTRX1.sizeT)return;
    for(i=0U;i<UARTRX1.num_datos;++i) {
        uint8_t c=UARTRX1.trama_rx[i];
        if(c=='$')usados=0U;
        else if(usados==0U)continue;
        if(usados>=sizeof(linea)-1U){usados=0U;continue;}
        linea[usados++]=(char)c;
        if(c=='\n') {
            linea[usados]=0;
            if(comando_valido(linea))responder();
            usados=0U;
        }
    }
}
