# Instrucciones para estacion de tierra tras USV 14.2
Proyecto tierra: STM32H562RGT6, archivo USV_CONTROLTIERRA_14_1_DIAGNOSTICO.zip. No se modifica aqui su firmware.

## XCTU y radio
- Leer y respaldar **ambas** radios reales por separado: SH/SL propio y DH/DL destino cruzados, HP, ID, CM, BD, NB, SB, AP, RO, RR, MR, TO, SM, D6/D7, potencia y firmware 8075.
- Captura: un XBee tiene SH/SL 0013A200419110BB y DH/DL 0013A20041867349. Perfil profile_8075.xpro apunta a 0013A20041911132. Direcciones distintas; no inferir la otra radio ni aplicar perfil parcial con imagen .ebin sin respaldo.
- En modo AT usar 115200 8N1, AP=0; mantener D6/D7 desactivados sin cableado RTS/CTS. TO=C0 DigiMesh es valido; luego de verificar las parejas y conservar respaldo, evaluar TO=40 punto-multipunto **en ambos** XBee y medir tasa de entrega/RTT.
- Registrar DB, ER, GD, EA, TR antes/despues de pruebas.

## Firmware tierra: acciones
1. Conservar UART5 PB13 TX / PB12 RX, 115200 8N1, RX por HAL_UART_Receive_IT, procesamiento de respuesta PUSVD y recuperación por USV_AsegurarRecepcionBote.
2. En Core/Src/main.c corregir rx5_silencio_ms: diag_rx5_ultimo_byte_ms SOLO se actualiza cuando llega byte real (HAL_UART_RxCpltCallback). Eliminar las asignaciones a ahora despues del rearme HAL_OK en USV_AsegurarRecepcionBote cerca de lineas 1375 y 1395, e inicializar al comenzar la aplicacion.
3. Instrumentar y comparar simultaneamente main_alive, pusvu_enviadas, tx5_intentos, tx5_busy, tx5_error, rx5_bytes, pusvd_validas, rx5_rearmes, rx5_watchdog, rtt_ultimo_ms y rtt_maximo_ms. En el USV revisar xbee_rx_* y xbee_uart1_error_irq.
4. Mantener de momento periodo PUSVU=400ms, ventana PUSVD=250ms, timeout TX=10ms; USV mantiene failsafe=500ms. NO prolongar el periodo de control sin verificar el failsafe y la prioridad de parada.
5. HAL_UART_Transmit HAL_OK no equivale a entrega RF ni ejecucion de parada; proteger la parada de emergencia, no bloquearla esperando telemetria.
6. Al cortar enlace, NO pulsar RESET 60s. Medir PB13 (TX tierra), PB12 (RX tierra), PA10 (RX USV), PA9 (TX USV), voltajes 3.3V de radios y guardar CSV simultaneos.
7. Prueba independiente de estribor: primero ESC desconectado, pulsos PB1/TIM3_CH4/J24 con osciloscopio. No interpretar promedio DC como ausencia de PWM.
8. Se requiere compilacion y validacion de la estacion antes de programar cambios.

## Estado
Son recomendaciones de implementacion y ensayo, no un firmware compilado ni reprogramacion remota de XBee.
