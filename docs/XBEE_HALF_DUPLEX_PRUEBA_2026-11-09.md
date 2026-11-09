# Ensayo de XBee half-duplex — 9 de noviembre de 2026

## Referencia y reversión

- Base que el equipo reportó satisfactoria: `6810474c2834437f3f958913669b0d42e4ece9da` (23/10/2026).
- Respaldo independiente: rama `respaldo/prueba-satisfactoria-2026-10-23`.
- Los cambios de enlace están en una rama de prueba. No modificar `main` hasta probar físicamente.

## Qué recibimos

- `test_bidireccional_at_usv2.zip`: script de PC que emite una orden antigua `$PUSVU,1500.0,1500.0,+00,01*42\r\n` incompatible con el STM32H562.
- `Xbee_bidireccional_at_v2.zip`: proyecto STM32F411CEU6 de ensayo que genera telemetría **fija**, no medidas reales. Sus 130 B tienen CRC `*4F` incorrecto; con CRC-8 polinomio 07 e inicio 00 la misma carga debe terminar en `*F8`.
- No contamos con perfiles XCTU exportados. No es posible afirmar ajuste perfecto de radio, firmware de antenas, payload RF, RO o reintentos.

## Firmware STM32H562 en esta revisión

- Se conserva el archivo `.ioc` íntegro y con los mismos periféricos; las únicas modificaciones físicas visibles son etiquetas descriptivas `XBEE_TX_USART1` (PA9) y `XBEE_RX_USART1` (PA10).
- USART1 se mantiene en modo asincrónico `TX_RX`, 115200 8N1. No utilizar HAL_HalfDuplex_Init con radios DIN/DOUT.
- DMA+IDLE se interpreta como **fragmento**, no trama final. Se copia el evento usando `num_datos`, se rearma RX, se acumula hasta `\r\n` y se valida `$PUSVU` con cinco campos enteros en límites permitidos.
- Si se recibió una orden válida se programa una respuesta `$PUSVD,...*HH\r\n` con CRC-8 luego de 60 ms como ventana de guarda inicial (por medir). El envío automático cada 500 ms queda deshabilitado en `main.c` para impedir tráfico no solicitado.
- Buffers USART1 y USART2 de DMA pasan a almacenamiento estático de 601 bytes, sin `malloc`, y las banderas de interrupción se declaran `volatile`.
- `PCB_BANCO_ACTIVO=1` **permanece activado**. No accionar motores ni servo desde RF en esta prueba. El failsafe de navegación sigue configurado a 500 ms en modo de navegación.
- La tarea periódica de la librería de telemetría todavía existe para compatibilidad, pero no se llama desde `main` en esta rama.

## Procedimiento mínimo

1. Mantener motores/servos desconectados de potencia. Compilar/cargar rama de prueba en H562.
2. Verificar en XCTU (para ambos radios) **modelo, firmware, AP, BD, NB, SB, SH/SL, DH/DL, ID, RO, NP, RTS/CTS, reintentos y cifrado** según familia de XBee. No inferir ajustes del ZIP.
3. Ejecutar `python tools/test_xbee_half_duplex.py --self-test`.
4. Con radio en puerto COM4: `python tools/test_xbee_half_duplex.py --port COM4 --cycles 120 --timeout 2.0 --interval 0.4`. Instalar `pyserial` primero.
5. Verificar respuestas `$PUSVD` con CRC correcto, tiempos, pérdida y recuperación de errores, tramas fragmentadas y comandos no válidos.
6. Ensayar el STM32F411 del ZIP solo como **responder sintético** usando la versión corregida entregada, nunca como reemplazo del USV productivo.

## Advertencias aún pendientes de prueba

- 60 ms **no garantiza** ausencia de RF en el aire. Medir con XCTU y logs de radios; ajustar tiempo y periodos de prueba.
- La solicitud `$PUSVU` no tiene CRC ni secuencia por compatibilidad heredada; la aplicación no puede demostrar entrega exactamente-una-vez ni identificar respuestas tardías. No usar el RTT como prueba inequívoca de correspondencia si el enlace pierde tramas.
- No hay garantía RF de un solo paquete; una línea de telemetría puede dividirse en varios paquetes transparentes.
- Verificar aparte que los campos de GNSS/temperatura/inundación transmitidos se actualicen desde sensores reales: el CRC de una trama no valida autenticidad de las mediciones.
- Sin hardware y perfiles XCTU no se ha hecho prueba de alcance ni de saturación; este commit es para ensayo, no aprobación de uso operativo.

## Fuentes
- Archivos de los dos ZIP entregados por el equipo.
- Firmware original H562 en el commit de respaldo indicado.
- Digi, manuales de modo transparente, paquetización RO/NP y buffers (consultar manual del modelo real antes de cambiar radios).
