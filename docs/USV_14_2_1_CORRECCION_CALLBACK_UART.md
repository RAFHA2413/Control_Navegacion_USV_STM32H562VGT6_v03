# 14.2.1 — Correccion de enlazado HAL_UART_ErrorCallback

Compilacion en VS Code fallaba en el enlazador por doble definicion:
- `Core/LIBRERIAS/pcb_banco.c` tiene el callback original para USART6 (banco/Teleplot).
- `Core/LIBRERIAS/UART_INT_DMA_IDLE.c` agrego erroneamente otro callback para USART1 durante la revision 14.2.

Este commit unifica la definicion del callback en `pcb_banco.c`:
- USART1/XBee: registra `ErrorCode`, incrementa `usv_xbee_uart1_error_irq` y marca `usv_xbee_uart1_recuperar` para gestionar DMA en `main.c`.
- USART6: mantiene exactamente la recuperacion anterior de recepcion por interrupcion.
- Las variables globales de diagnostico permanecen definidas una sola vez en `UART_INT_DMA_IDLE.c`.
- No cambia motores, PWM, protocolo ni failsafe.
- Pendiente comprobacion de enlazado en herramienta ARM GCC del usuario y ensayos de banco sin potencia de motores.
