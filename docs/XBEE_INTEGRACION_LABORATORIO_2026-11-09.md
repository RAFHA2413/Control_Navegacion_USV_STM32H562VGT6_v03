# INTEGRACION FINAL DE LABORATORIO — XBee AT / STM32H562
Fecha: 2026-11-09

Version de respaldo satisfactoria (segun equipo): `6810474c2834437f3f958913669b0d42e4ece9da`, rama `respaldo/prueba-satisfactoria-2026-10-23`.
Base de half-duplex: `ba344439f895c3dc8debcac49bd9ec349c0cbfd0`.

## Cambios
- `PCB_BANCO_ACTIVO=0`: habilita PWM de ambos motores y del servo de camara cuando se reciben comandos `$PUSVU,babor,estribor,camara,bomba,parada\r\n` validos.
- Escala de motores: -100 a +100 (PWM segun calibracion existente); servo: -90 a +90 grados.
- PB5 bomba activa en LOW; apagada en arranque, cuando `parada=1` o al vencer el failsafe de 500 ms. Se ejecuta `bomba` solo con comando valido.
- El servicio XBee responde una `$PUSVD` con CRC-8 por cada orden valida; UART1 115200 8N1, PA9 TX y PA10 RX; no se cambia a modo UART half-duplex de un hilo.
- GNSS L76K (USART2) volcado al objeto de telemetria cuando RMC/GGA son validas; los datos GNSS caducan a los 3 s. Se restituye hora UTC con la conversion local existente de gps.c. Velocidad se envia en nudos.
- Temperatura DS18B20 se agrega solo si esta en el rango valido de Trama V3 (-10 a 85 C). Sin lectura valida, queda la ultima lectura conocida: el protocolo no aporta bit de calidad de temperatura.
- IMU/voltaje/corriente mantienen codigo de integracion previo.

## Limitaciones de la prueba
- El segundo ZIP contiene un STM32F411 **respondedor de ensayo**, NO el firmware de consola Nextion de la estacion de tierra. No se alteran ni verifican controles, pantallas ni configuracion XCTU del equipo real.
- La trama anterior del script Python `$PUSVU,1500.0,1500.0,+00,01*42` es incompatible y NO se debe enviar al USV. Enviar el formato de cinco valores enteros que se muestra arriba.
- El campo `inundacion` sigue sin fuente identificada en el hardware/firmware y permanece con el valor inicial; no considerarlo lectura real validada. El estado de luces y modo solicitado tampoco esta integrado a las ordenes actuales.
- Las radios half-duplex RF no requieren GPIO/USART half-duplex del STM32. La logica de solicitud-respuesta evita TX autonomas, pero no prueba que los radios no colisionen ni que la ventana de 60 ms sea suficiente. Modelo, firmware y parametros XCTU (AP, BD, NP, RO, DH/DL, ID, RTS/CTS) requieren inspeccion en los equipos.
- La orden PUSVU NO tiene CRC ni secuencia en el protocolo vigente. Un CRC valido de PUSVD no prueba fidelidad fisica del enlace, ni que la trama contenga solamente medidas actuales.
- No se ha ejecutado prueba fisica, ni compilacion completa con arm-none-eabi-gcc: este commit es candidato a la integracion inmediata, no certificacion del enlace.

## Secuencia antes de habilitar la potencia
1. Mantener propulsores fuera del agua y libres de personas/herramientas, asegurados contra giro inesperado. Verificar parada fisica (no solo digital) y posibilidad de desconectar la alimentacion.
2. Confirmar reloj, PWM en neutro, cableado y el puerto RX/TX correcto. Comprobar salida servo en rango seguro.
3. Configurar XBee AT transparentes con mismos parametros; comprobar que controlador real de tierra envia cinco valores (sin CRC) y espera la respuesta completa PUSVD/CRC antes de nuevo comando.
4. Energizar actuadores solo cuando las comprobaciones previas sean satisfactorias. Primera orden recomendada: `$PUSVU,0,0,0,0,1\r\n` (parada).
5. Probar ordenes controladas y de baja potencia, luego comprobacion real de failsafe cortando la transmision de tierra por mas de 500 ms.
6. Registrar latencia del enlace, consistencia de la telemetria y cualquier error. Si falla, volver a rama de respaldo; conservar ambas referencias Git.

## Archivo .ioc
El `.ioc` vigente queda incluido en el proyecto completo: no cambiamos clocks, puertos, pinout ni DMA por activar el modo navegacion. Los cambios de banco/actuadores son de firmware en `pcb_banco.h` y `main.c`.
