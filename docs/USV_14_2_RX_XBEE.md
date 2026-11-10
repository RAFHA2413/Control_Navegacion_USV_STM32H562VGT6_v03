# USV 14.2 - Recuperacion de RX XBee
Base: commit 11643758a6d12e2c1f24fc3fb909978c5cf1bc78; firmware STM32H562VGT6.

- UART1 PA9 TX / PA10 RX permanece 115200 8N1, modo AT transparente del XBee (AP=0).
- UARTRX retorna HAL_StatusTypeDef y comprueba HAL_OK al rearmar DMA.
- HAL_UART_ErrorCallback USART1 contabiliza errores y marca recuperacion pendiente sin rearmar dentro de ISR.
- En main se recupera RX mediante HAL_UART_AbortReceive y ReceiveToIdle_DMA ante error o despues de 3000ms sin eventos, separado por 1000ms entre intentos.
- Teleplot USART6 PC6: xbee_rx_eventos, xbee_rx_bytes, xbee_rx_silencio_ms, xbee_rx_rearmes_ok, xbee_rx_rearmes_error, xbee_rx_watchdog, xbee_uart1_error_irq, xbee_uart1_error_codigo, xbee_rx_descartadas.
- Sin cambio de failsafe (500ms), guarda TX (60ms), protocolo PUSVU/PUSVD ni sentido/porcentaje de motores.
- NO se modifican parametros AT del XBee desde el STM32. Verificar ambas radios en XCTU antes de ensayo TO=40; conservar perfiles TO=C0 anteriores.
- Debe compilarse con ARM GCC y ensayarse en banco con motores sin potencia, Teleplot de tierra y USV simultaneos, 60s luego de un corte sin RESET.
- Resultado HAL_OK solo confirma UART/RX DMA local, no RF.
