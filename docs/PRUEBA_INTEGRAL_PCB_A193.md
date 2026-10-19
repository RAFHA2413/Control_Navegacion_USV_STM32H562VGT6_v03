# Protocolo de prueba integral USV 0067 / PCB A193

**Estado**: software preparado, NO compilado con toolchain STM32 ni probado fisicamente.
**Rama**: pruebas-integrales-pcb-20261019. Mantener MAIN para volver al programa anterior.

## Seguridad antes de energizar
1. Verificar visualmente soldaduras, orientacion de diodos/electroliticos y ausencia de cortos. Medir 3.3V y 5V con multimetro y usar fuente limitada en corriente para la primera prueba.
2. F1/D2 estan previstos para 5A; nunca alimentar por este trayecto los motores que han consumido 27A. ESC y motores requieren ruta de potencia separada y protecciones correctas, con referencia comun solo segun diseño.
3. Inmovilizar los propulsores, retirar helices si el fabricante lo permite y disponer de corte FISICO inmediato de energia de ESC. Un bloqueo del firmware podria dejar PWM activo.
4. Alimentar servo con fuente adecuada y topes libres, no desde el pin de 3.3V del micro. No probar bomba en seco.
5. Las DI sin polarizacion pueden dar lecturas flotantes. Las DO se prueban HIGH, por lo tanto verificar cargas y logica antes de activar.

## Conexion USB–TTL por J11
Pin 2 GND, pin 3 TX PC6 al RX de adaptador, pin 4 RX PC7 desde TX de adaptador. Pin 1 sin conexion. USB-TTL 3.3V logico, no alimentar PCB desde VCC USB-TTL. USART6 a 115200/8N1, sin flujo. Para mandar comandos usar terminal serial bidireccional con terminador de linea CR o LF; cerrar Teleplot si el COM esta ocupado. Solo una aplicacion puede abrir el COM a la vez.

## Comandos manuales, en MAYUSCULAS
| Comando | Resultado |
|---|---|
| PCB HELP | ayuda |
| PCB STATUS | estado actual |
| PCB ARM | habilita comandos de banco por 30 segundos; pone todos en reposo |
| PCB STOP | desarma, PWM motores neutro, servo centro, bomba y DO apagadas |
| PCB SERVO -20 | coloca camara a -20 grados; rango permitido -30..30 |
| PCB SERVO 20 | coloca camara a +20 grados |
| PCB MOTOR B 10 | pulso babor +10% hasta 700 ms y neutro automatico |
| PCB MOTOR E 10 | pulso estribor +10% hasta 700 ms |
| PCB MOTOR BOTH 10 | ambos motores +10% hasta 700 ms |
| PCB MOTOR B -10 | reversa, solo si ESC admite ese modo |
| PCB PUMP 100 | rele PB5 a LOW 100 ms, solo bomba sumergida |
| PCB DO1 100 | PB12 HIGH 100 ms; aplica tambien DO2, DO3, DO4 |

ARM nunca pone motores en movimiento; cada actuador exige comando manual y todos los comandos de actuacion requieren ARM vigente. Rango motores -15..15%; pulso 700 ms; bomba y DO max 200 ms. Orden desconocida desarma. Si el micro se detiene en un pulso, solo el corte de energia es proteccion real.

## Ensayo por fases
1. Sin motores ni bomba: Teleplot main_alive debe incrementar; registrar voltajes de 3.3V, 5V, bateria_voltaje_v; comparar con multimetro.
2. IMU: imu_ok/imu_data_ok, roll/pitch/yaw cambiando al inclinar; temperatura_c DS18B20; humedad_pct; GNSS gps_fix/gps_satelites en sitio con recepcion.
3. Corriente: corriente_a LEM contrastada con medicion real y calibracion; entradas di1..di4 alternadas de forma segura y humedad_do.
4. Servo alimentado correctamente: PCB ARM, PCB SERVO -20, PCB SERVO 20, PCB STOP.
5. ESC con potencia independiente de D2/F1 y con corte fisico, motores asegurados: PCB ARM, PCB MOTOR B 10, PCB MOTOR E 10, PCB STOP; posteriormente BOTH, si cada etapa paso.
6. Verificar salidas DO y bomba solo si cableado, cargas, consumo y condiciones fisicas son aptos.
7. Guardar registro de pruebas reales con fecha, estado, voltajes, PWM, capturas de Teleplot y fallos. No atribuir ensayos no realizados.
8. Tras esta fase, volver al firmware para estacion de tierra: protocolo PUSVU por USART1/XBee y telemetria PUSVD, parada de emergencia y failsafe 500 ms.

## Alcance
Esta rama impide que los comandos de la estacion por USART1 accionen motores y servo; sigue leyendo entradas y enviando telemetria. Teleplot agrega pcb_armado, pcb_motor_pulso, pcb_bomba_pulso, pcb_do_pulso, di1..di4, humedad_do. Solo firmware de ensayo; no usar para navegacion.

**Servo al encender:** en esta rama no se inicia PWM TIM3_CH3 al arrancar. La primera orden PCB SERVO, posterior a PCB ARM, habilita el PWM y posiciona la camara; PCB STOP devuelve al centro. Una vez habilitado PWM durante la sesion, un reinicio corta la salida hasta una nueva orden.
