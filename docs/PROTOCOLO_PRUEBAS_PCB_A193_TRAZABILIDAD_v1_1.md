# USV 0067 — PCB A193: protocolo de pruebas y trazabilidad (v1.1 — actualizado desde commits)

**Fecha:** 2026-10-19 · **Estado:** PROPUESTA; no aprobada formalmente por director, sin resultados experimentales incorporados.  
**Documento maestro:** `Protocolo_Verificacion_Integral_PCB_USV_0067_A193_v1_1.docx`, 14 páginas, elaborado como anexo independiente.  
**SHA-256 del Word v1.0:** `1eccfdac5641248364cde586829eef06e3019170a954423374216e8cff468927`.  
**Firmware de referencia:** `4a68a5cc15bc33537b45516bf5b4d4f4461f15c6` (modo banco 13.2).  
**Proyecto:** STM32H562VGT6; esquemático y PCB `USV_VERSION_FINAL` en `REVISION ALCIDES No. 3.zip`, revisión física a contrastar con A193 fabricada.

Este archivo es un **registro resumido y versionable en GitHub** actualizado con los commits originales. El Word maestro contiene las instrucciones detalladas, tablas, referencias APA y formularios en blanco. Este archivo no debe confundirse con una constancia de que se completaron los ensayos.

## Condiciones previas y seguridad

1. Inspección visual y continuidad; fuente limitada y verificación medida de 3,3 V y 5 V.
2. Compilar y grabar firmware de la revisión; registrar build, binario, herramienta y SHA.
3. Usar USART6 J11: pin 1 NC, 2 GND, 3 PC6/TX, 4 PC7/RX. USB–TTL lógico de 3,3 V, puerto a 115200/8N1, fin de línea CR/LF.
4. **Los motores NO se alimentan por F1/D2 nominales de 5 A**; requieren ESC y circuito de potencia separado, protecciones correctas y desconexión física de emergencia.
5. Mantener propulsores inmovilizados, sin hélices si procede según fabricante; probar primero señales PWM sin motor energizado.
6. Servo con fuente adecuada independiente; bomba solo en condiciones admitidas por fabricante, nunca en seco.
7. Detener ante reinicio, caída de tensión, sobrecorriente, sobretemperatura, movimiento imprevisto o pérdida de control.
8. Las salvaguardas por software no sustituyen paro eléctrico independiente, porque pueden dejar de operar si el MCU se bloquea.

## Matriz preliminar requisito -> caso -> evidencia

**ATENCIÓN:** Los IDs R-01...R-19 son identificadores de trabajo del protocolo y ya tienen correspondencia técnica con subsistemas y commits verificables, conforme a la tabla que sigue. No representan números de las 54 preguntas del formulario: no se dispone aquí de cada ítem original para efectuar esa segunda vinculación documental.

| Req. | Caso | Subsistema / verificación | Evidencia |
|---|---|---|---|
| R-01 | P-01 | Soldaduras y alimentación | Inspección y lecturas multímetro |
| R-02 | P-02 | Compilación, grabación, actividad | Log de build y main_alive |
| R-03 | P-03 | UART6, Teleplot y recepción de comandos | Log UART, respuesta PCB STATUS |
| R-04 | P-04 | IMU BNO085, roll/pitch/yaw | Ángulos de referencia y medidos |
| R-05 | P-05 | GNSS / USART2 / 9600 | GNSS_rx, satélites, lat/long, fix |
| R-06 | P-06 | DS18B20 | temperatura_c vs termómetro |
| R-07 | P-07 | Humedad analógica y digital | humedad_adc, humedad_pct, humedad_do |
| R-08 | P-08 | Medición de batería | bateria_voltaje_v vs multímetro |
| R-09 | P-09 | Sensor LEM | corriente_a vs pinza |
| R-10 | P-10 | DI1–DI4 / PA4–PA7 | Entradas 0 y 3,3 V |
| R-11 | P-11 | DO1–DO4 / PB12–PB15 | Pulsos y retorno LOW |
| R-12 | P-12 | Servo / PB0 / TIM3_CH3 | Posiciones −20°, 0°, +20° y PWM |
| R-13 | P-13 | Motor babor / PB4 / TIM3_CH1 | PWM y retorno a neutro |
| R-14 | P-14 | Motor estribor / PB1 / TIM3_CH4 | PWM y retorno a neutro |
| R-15 | P-15 | Coordinación de dos motores | PWM de ambos, corriente, tensión |
| R-16 | P-16 | Bomba / PB5 | Pulso, relé y reposo |
| R-17 | P-17 | Parada, ARM y caducidad | PCB STOP, timeout, rechazo sin ARM |
| R-18 | P-18 | Integración sensores y actuadores | Telemetría y condiciones bajo carga |
| R-19 | P-19 | Acta de liberación a estación de tierra | Acta firmada e incidencias cerradas |


## Matriz de hardware y firmware comprobable por historial de commits

Se consultaron el código actual, la configuración STM32CubeMX, las bibliotecas específicas y el historial Git del mismo repositorio. Los SHA son prefijos de commits **existentes**; documentan implementación y evolución del software, no equivalen a ensayos aprobados en la PCB física. El Word v1.1 contiene la tabla A2a y los instrumentos, métodos y hojas de resultados.

| Equipo/subsistema | Interfaz e implementación registradas | Commits de referencia | Caso / estado |
|---|---|---|---|
| PCB y alimentación | D1/D2/F1, 12V_PROT, 5V_SYS, 3V3 | Esquemático/PCB A193 | P-01: mediciones pendientes |
| Microcontrolador | STM32H562VGT6, `main_alive`, ADC1/DMA/TIM3 | 41ed7d1b (11.4), 4a68a5cc (13.2) | P-02: build/prueba pendientes |
| Diagnóstico | USART6 PC6 TX, PC7 RX, J11; 115200/8N1 | c64df319 (13.0), 4a68a5cc (13.2) | P-03 |
| IMU | BNO085, driver BNO080/BNO085; I²C1 PB6/PB7 | 06136db5 (4), fed1327c | P-04 |
| GNSS | L76K, USART2 PA2/PA3, sentencias RMC/GGA | db57b2a3 (5.1), 6eb2651d (5.19) | P-05 |
| Temperatura | DS18B20, 1-Wire PC2, J22 | 02f625ab (2.1), 86cbe5c0 (2.3) | P-06 |
| Humedad | ADC1 PC3 y digital PC4; modelo no especificado en el código | 0f9f551b (1.4), 9bcc61b1 (1.6) | P-07 |
| Batería | Divisor nominal 20 kΩ/4,7 kΩ, ADC1 PC1 | 35ceed13 (8.1), 5b8bb86e (9.1) | P-08 |
| Corriente | LEM HASS 200-S, ADC1 PA0 | 024d339f (7.1), 4a9b2818 (7.3) | P-09 |
| Entradas digitales | DI1–DI4: PA4–PA7, J15 | main.h; c64df319 (13.0) | P-10 |
| Salidas digitales | DO1–DO4: PB12–PB15, pulsos | main.h; c64df319 (13.0) | P-11 |
| Servo de cámara | MG996R, PB0/TIM3_CH3 | 4ce337a6 (3.1), 474c7bcc | P-12 |
| ESC babor | PB4/TIM3_CH1, neutro 1500 µs | 735b16c6 (6.3), 05d958e6 (12.0) | P-13 |
| ESC estribor | PB1/TIM3_CH4, neutro 1500 µs | 026ea0d1 (6.4), 05d958e6 (12.0) | P-14 |
| Propulsión conjunta | Dos canales ESC, banco ±15%, pulsos 700 ms programados | 588f1559 (6.5), c64df319 (13.0) | P-15 |
| Achique | PB5 open-drain, relé activo LOW | cea7bf11 (10.1), c055438c (10.3) | P-16 |
| Seguridad de banco | `PCB ARM`, `PCB STOP`, desarme/neutralización | c64df319, 4f696e3a, 4a68a5cc | P-17 |
| Integración | Sensores + telemetría + actuadores | 41ed7d1b (11.4), c64df319 (13.0) | P-18 |
| Radio y estación de tierra | XBee, USART1 PA9/PA10; comandos `$PUSVU`, telemetría `$PUSVD` | ba51d6f1 (11.1), 05d958e6 (12.0) | P-19: liberación, no ensayo remoto aprobado |
| Pantalla Nextion | USART3 PD8/PD9 y `BATERIA_Actualizar_Nextion`; sin llamada activa en `main.c` revisado | aaa55467 (8.2); `bateria_motores.c` | Integración/prueba específica pendiente |
| Vídeo de cámara | El STM32 gobierna el servo; no se identifica procesamiento/transmisión de vídeo en firmware revisado | `servos.c`; interfaz externa no comprobada | Fuera del banco actual; no declarado probado |

**Límite documental:** los cuatro equipos del formulario de 54 preguntas son antecedentes de los requerimientos levantados por el proyecto, pero el GitHub de firmware no contiene correspondencia pregunta-a-requisito. Por eso la matriz aquí es trazabilidad **de implementación** y no una reconstrucción especulativa de los datos de encuesta. Los ensayos reales continúan **sin diligenciar**.

## Restricciones cuantitativas documentadas en el firmware

- UART6 = **115200, 8N1**; Teleplot emite variables con prefijo `>variable:valor`, nominalmente a 2 Hz.
- Modo banco: `PCB ARM` caduca a **30 s** y deja actuadores en reposo al activar; `PCB STOP` desarma y ordena reposo.
- Servo: `PCB SERVO -20`, `PCB SERVO 0`, `PCB SERVO 20`; rango de banco ±30°. PWM teórico para esos ángulos ≈1278/1500/1722 µs; verificar con instrumento.
- Motores: `PCB MOTOR B 10`, `PCB MOTOR E 10`, `PCB MOTOR BOTH 10`; rango limitado a ±15%, duración programada hasta 700 ms y retorno previsto a neutro (1500 µs).
- Bomba: `PCB PUMP 100`, PB5 activa nivel bajo, temporizada ≤200 ms.
- Salidas: `PCB DO1 100` a `PCB DO4 100`, nivel HIGH, temporizadas ≤200 ms.
- El retorno automático depende de la ejecución normal del firmware; nunca sustituye corte físico.
- **Por aprobar antes de medir:** tolerancias de tensión/corriente, error angular, error de temperatura/humedad, error GNSS, repetibilidad, duración real del PWM y límites bajo carga. No se atribuyen estos umbrales a ISO, IPC ni IEC.

## Registro experimental exigido por caso

- ID P-xx, fecha/hora, lugar y condiciones.
- PCB física, revisión, firmware SHA, instrumentos/estado de calibración.
- Preparación y criterio de aceptación definido **antes** de medir.
- Magnitud de referencia con unidad, medición USV con unidad, repetición, error absoluto y relativo cuando corresponda.
- Archivo de evidencia original (CSV, fotografías, captura de osciloscopio, Teleplot).
- Desviación, acción correctiva, commit de corrección, repetición y decisión: APROBADO / NO APROBADO / NO EJECUTADO / NO EVALUABLE.
- Consolidado final de 19 casos y acta de liberación firmada.

## Marco bibliográfico de referencia

- ISO/IEC/IEEE 15288:2023 (procesos del ciclo de vida): https://www.iso.org/standard/81702.html
- ISO/IEC/IEEE 29119-2:2021 (procesos de prueba del software): https://www.iso.org/standard/79428.html
- ISO/IEC/IEEE 29119-3:2021 (documentación de pruebas): https://www.iso.org/standard/79429.html
- IPC-A-610J y J-STD-001J, revisión de 2024, para prácticas de ensamblaje: https://www.ipc.org/news-release/ipc-releases-j-revisions-two-leading-standards-electronics-assembly
- STM32H562 RM0481/DS14258/ES0565: https://www.st.com/en/microcontrollers-microprocessors/stm32h562/documentation.html
- IEC 60945:2002 (contexto naval para futuras pruebas ambientales y EMC, NO aplicada como certificación): https://webstore.iec.ch/en/publication/3959

Las referencias ISO/IEC/IEEE e IPC son un marco técnico-metodológico, no una declaración de conformidad a cláusulas sin disponer de los textos normativos y sus criterios. La versión detallada con referencias APA y hojas de ejecución es el Word identificado al inicio.

## Próximo hito

Revisión del protocolo por el director; vinculación R-01...R-19 con requisitos originales; fijación y aprobación de tolerancias; ejecución P-01 a P-18; cierre de incidencias; acta P-19; después plan y ensayos de estación de tierra `$PUSVU/$PUSVD` por USART1/XBee con parada remota y failsafe.
