# USV 14.1 — Correccion PUSVD y diagnostico TX (09-11-2026)

## Procedencia

Rama basada directamente en el commit USV 14.0 `71b2ed11f8a5dd62c77a70299c9700891f83714f`. Esta revision corresponde **solo al microcontrolador del USV (STM32H562VGT6)**, no al firmware de la estacion de tierra.

## Evidencia y motivo

En el registro bruto de tierra `prueba 4.log` del proyecto proporcionado se detectaron campos numericos recibidos como `ld.ld` en tramas `$PUSVD`, que la estacion rechazo por formato/campos. La funcion `USV_FormatearDecimalX10()` usaba `%lld` con `newlib-nano` (`--specs=nano.specs`), causa probable y reproducible del problema.

## Cambios en este commit

1. `Core/LIBRERIAS/trama_usv.c`: sustituye `%lld` por `%lu` con conversion explicita a `unsigned long`. Mantiene el signo y un decimal; los valores de entrada son `int32_t` y la magnitud nunca excede 2.147.483.648, representable con `unsigned long` de 32 bits.
2. `Core/Src/main.c`: incorpora cuatro indicadores en Teleplot por USART6, cada 500 ms junto con los demas diagnosticos:
   - `pusvd_tx_uart_ok`: cantidad de transmisiones USART1 que devolvieron `HAL_OK`.
   - `pusvd_tx_uart_err`: errores al transmitir por USART1.
   - `pusvd_tx_formato_err`: fallos al construir la trama.
   - `pusvd_tx_ultimo_bytes`: longitud de la ultima trama construida.

**Precaucion:** `HAL_OK` no equivale a entrega por radio ni a aceptacion de telemetria en tierra; el criterio remoto es `pusvd_validas`.

## Elementos que NO se modificaron

- Sentidos de giro, motor babor PB4/TIM3_CH1, estribor PB1/TIM3_CH4, servo y bomba.
- Parada de emergencia, neutro PWM, ni failsafe de 500 ms.
- Configuracion XBee USART1 y espera de respuesta RF.
- Firmware de la estacion de tierra (requiere su propia compilacion y revision).

## Pruebas requeridas (aun pendientes)

1. Compilar en PC con el toolchain ARM utilizado para la version 14.0: `cmake --preset Debug` y `cmake --build --preset Debug`.
2. En banco y sin potencia en propulsores, registrar Teleplot en USART6/PC6 y en la estacion de tierra; verificar que `pusvd_tx_uart_ok` aumente al recibir `$PUSVU` y que `pusvd_tx_formato_err` permanezca en cero.
3. Validar en tierra que `pusvd_validas` aumente y `pusvd_campos_err` deje de crecer por el defecto `ld.ld`. Si no ocurre, capturar una `TRAMA_RX` bruta para identificar el campo que falla.
4. Verificar que una perdida de comunicaciones mantenga la propulsión en estado seguro; repetir en distintas condiciones sin energizar los motores en la fase diagnostica.
5. Probar de manera independiente orientacion de babor y salida PB1/ESC estribor antes de usar propulsores reales.

## Estado

Cambio de codigo identificado y preparado, **sin compilacion ARM completa ni validacion en hardware por este commit**. No habilitar pruebas de mar hasta finalizar pruebas de banco y parada fisica.
