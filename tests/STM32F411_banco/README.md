# STM32F411 CEU6 de ensayo
Archivo de reemplazo: `Core/Librerias/UARTRX.c` del ZIP `Xbee_bidireccional_at_v2.zip`.
Mantener el resto del proyecto original (incluido `Xbee_bidireccional_at_v2.ioc`, USART1 115200 8N1, DMA2_Stream2).
**Este F411 actua como RESPONDEDOR de pruebas**, no contiene la estacion de control definitiva.
La trama `$PUSVD` generada contiene valores fijos **sinteticos**, con CRC generado correctamente (F8); no es telemetria real.
Para usar la version completa corregida, utilizar el ZIP de prueba actualizado proporcionado con esta revision.
