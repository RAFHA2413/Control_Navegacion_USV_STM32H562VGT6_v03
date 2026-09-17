/*
 * trama_usv.h
 *
 * Librería para construir, verificar y decodificar
 * las tramas de comunicación entre:
 *
 * - Control de tierra STM32H562RGT6
 * - Radio XBee
 * - Microcontrolador instalado en el bote
 *
 * La trama de comandos enviada desde tierra comienza con:
 *
 * $PUSVU
 *
 * La trama de telemetría recibida desde el bote comienza con:
 *
 * $PUSVD
 *
 * Ambas tramas terminan con:
 *
 * *HH\r\n
 *
 * HH corresponde al CRC-8 escrito en hexadecimal.
 */

#ifndef TRAMA_USV_H_
#define TRAMA_USV_H_

/* Incluye los tipos y definiciones generales del proyecto STM32. */
#include "main.h"

/* Define el tipo size_t utilizado para indicar tamaños de buffers. */
#include <stddef.h>

/* Define los tipos uint8_t, uint16_t, int16_t y uint32_t. */
#include <stdint.h>

/*
 * Asignacion vigente despues del intercambio de pines:
 *
 * UART5  (&huart5): trama hacia el bote, PB13 TX y PB12 RX.
 * USART1 (&huart1): pantalla Nextion, PA9 TX y PA10 RX.
 *
 * Las referencias a USART1 como transmisor de la trama en los
 * comentarios originales corresponden a la asignacion anterior.
 * Esta libreria construye y decodifica texto; no selecciona la UART.
 */


/*
 * Tamaño máximo reservado para una trama completa.
 *
 * Incluye:
 *
 * - Identificador de la trama.
 * - Todos los campos.
 * - Separadores.
 * - CRC.
 * - Retorno de carro y salto de línea.
 * - Carácter nulo final.
 */
#define USV_TRAMA_MAXIMA             256U


/*
 * Cantidad máxima de campos que puede separar el decodificador.
 *
 * La trama de telemetría $PUSVD utiliza actualmente 31 campos,
 * incluyendo el identificador de la trama.
 */
#define USV_MAXIMO_CAMPOS             32U


/*
 * Tamaño reservado para almacenar la hora UTC.
 *
 * Formato esperado:
 *
 * hhmmss.ss
 *
 * Ejemplo:
 *
 * 153025.50
 */
#define USV_TAMANO_UTC                16U


/*
 * Tamaño reservado para almacenar una coordenada GPS.
 *
 * La coordenada se conserva como texto para evitar perder
 * precisión durante la comunicación.
 */
#define USV_TAMANO_COORDENADA         20U


/*
 * ORDEN DE LOS VALORES EN LA TRAMA DE COMANDOS $PUSVU
 *
 * Despues del identificador se envian exactamente 16 valores.
 * Los nombres indicados aqui sirven como documentacion:
 * no se transmiten etiquetas, nombres de variables ni signos '='.
 *
 * Posicion  1: secuencia.
 * Posicion  2: potencia_global_x10.
 * Posicion  3: direccion_x10.
 * Posicion  4: potencia_babor_x10.
 * Posicion  5: potencia_estribor_x10.
 * Posicion  6: camara_x10.
 * Posicion  7: babor_avante.
 * Posicion  8: babor_atras.
 * Posicion  9: estribor_avante.
 * Posicion 10: estribor_atras.
 * Posicion 11: luces.
 * Posicion 12: bomba.
 * Posicion 13: reconexion.
 * Posicion 14: modo_manual.
 * Posicion 15: parada.
 * Posicion 16: falla_direccion.
 *
 * Los valores analogicos se transmiten como enteros multiplicados
 * por 10, conservando una cifra decimal sin enviar un punto:
 *
 * 500  representa 50.0 % de potencia.
 * -450 representa -45.0 grados de camara.
 *
 * La secuencia no se multiplica por 10.
 * Las entradas digitales se transmiten como 0 o 1.
 *
 * Ejemplo completo, con su CRC-8:
 *
 * $PUSVU,1,500,0,500,500,-450,1,0,1,0,1,0,0,1,0,0*5A\r\n
 *
 * Al enviar, \r\n representa los bytes 0x0D y 0x0A;
 * no se envian las barras ni las letras de esa representacion.
 * El caracter nulo '\0' termina la cadena en memoria y no se envia.
 *
 * El bote debe utilizar el mismo orden y la misma escala.
 * El formato anterior con etiquetas no es compatible con este.
 * Se conserva el formato actual de telemetria $PUSVD.
 */

/*
 * ACTUALIZACION: CAMARA EN GRADOS ENTEROS SIN PUNTO DECIMAL
 *
 * El bloque anterior se conserva como documentacion original;
 * su escala y su ejemplo de camara corresponden a la version anterior.
 *
 * Ahora la posicion 6 de la trama contiene grados enteros de -90 a +90:
 *
 * -90 = -90 grados.
 * -45 = -45 grados.
 *   0 = posicion central.
 *  45 = +45 grados.
 *  90 = +90 grados.
 *
 * No se envia un punto decimal ni el nombre de la variable.
 * Los positivos se transmiten sin el signo '+'.
 *
 * Ejemplo actualizado, con su CRC-8:
 *
 * $PUSVU,1,500,0,500,500,-45,1,0,1,0,1,0,0,1,0,0*06\r\n
 *
 * Solamente cambia la escala transmitida de la camara.
 * Las potencias y la direccion conservan sus valores x10.
 * El orden, la secuencia, los digitales, el CRC-8 y la terminacion
 * permanecen iguales. La telemetria $PUSVD tampoco cambia.
 *
 * El receptor del bote debe utilizar esta nueva escala de camara.
 * No debe interpretar los grados recibidos como decimas de grado.
 */

/*
 * ACTUALIZACION VIGENTE: COMANDO SIN ESCALA X10
 *
 * Los dos bloques anteriores se conservan como historial.
 * Sus nombres, escalas y ejemplos corresponden a versiones anteriores.
 *
 * Ahora el comando utiliza las mismas unidades en memoria y por serial.
 * No se multiplica ni se divide por 10 ningun campo de USV_Comando.
 *
 * Orden vigente despues del identificador $PUSVU:
 *
 * Posicion  1: secuencia.
 * Posicion  2: potencia_global, porcentaje entero de 0 a 100.
 * Posicion  3: direccion, demanda de giro entera de -100 a +100.
 * Posicion  4: potencia_babor, porcentaje entero de 0 a 100.
 * Posicion  5: potencia_estribor, porcentaje entero de 0 a 100.
 * Posicion  6: camara, grados enteros de -90 a +90.
 * Posicion  7: babor_avante.
 * Posicion  8: babor_atras.
 * Posicion  9: estribor_avante.
 * Posicion 10: estribor_atras.
 * Posicion 11: luces.
 * Posicion 12: bomba.
 * Posicion 13: reconexion.
 * Posicion 14: modo_joystick.
 * Posicion 15: parada.
 * Posicion 16: falla_direccion.
 *
 * Las posiciones 7 a 16 admiten solamente 0 o 1.
 *
 * Ejemplo vigente, con su CRC-8:
 *
 * $PUSVU,1,50,0,50,50,-45,1,0,1,0,1,0,0,1,0,0*18\r\n
 *
 * 50 significa directamente 50 %.
 * -45 significa directamente -45 grados de camara.
 * No se transmiten simbolos de porcentaje ni puntos decimales.
 *
 * modo_joystick = 0:
 *
 * - Control manual mediante los potenciometros independientes.
 * - potencia_global y direccion se envian en cero.
 * - Cada potenciometro determina la potencia de su propio motor.
 *
 * modo_joystick = 1:
 *
 * - Control mediante potenciometro general y joystick de direccion.
 * - Los potenciometros independientes no gobiernan la propulsion.
 * - El general establece el limite maximo para cada motor.
 * - Giro positivo: derecha; babor conserva potencia y estribor disminuye.
 * - Giro negativo: izquierda; estribor conserva potencia y babor disminuye.
 *
 * main.c selecciona los controles y calcula las potencias finales.
 * Esta libreria no realiza esa seleccion ni calcula la mezcla.
 * El receptor aplica potencia_babor y potencia_estribor sin repetirla.
 *
 * La camara permanece disponible en ambos modos.
 * La parada de emergencia tiene prioridad sobre la propulsion.
 *
 * Se mantienen los dieciseis valores, el algoritmo CRC-8 y CR/LF.
 * La telemetria $PUSVD mantiene sus campos y escalas actuales.
 *
 * El transmisor y el receptor deben utilizar esta misma version.
 * Un receptor antiguo puede interpretar mal las unidades aunque
 * el CRC de la trama recibida sea correcto.
 */


/*
 * Estructura que contiene todas las órdenes enviadas
 * desde el control de tierra hacia el bote.
 *
 * Esta información será enviada por USART1 mediante los XBee.
 */
/*
 * Asignacion vigente: esta estructura se transmite por UART5.
 * Los comentarios originales de escala se conservan debajo.
 * Los comentarios de actualizacion indican las unidades vigentes.
 */
typedef struct
{
    /*
     * Número consecutivo de la trama.
     *
     * Permite identificar:
     *
     * - Tramas perdidas.
     * - Tramas repetidas.
     * - Reinicios del transmisor.
     */
    uint32_t secuencia;

    /*
     * Potencia general solicitada.
     *
     * Se almacena multiplicada por 10.
     *
     * Rango:
     *
     * 0     = 0.0 %
     * 1000  = 100.0 %
     */
    /*
     * Vigente: porcentaje entero de 0 a 100, sin escala x10.
     * Se utiliza como limite por motor solamente en modo joystick.
     * En modo manual independiente se envia en cero.
     */
    uint16_t potencia_global;

    /*
     * Dirección solicitada mediante el joystick del bote.
     *
     * Se almacena multiplicada por 10.
     *
     * Rango:
     *
     * -1000 = giro máximo hacia un lado.
     *     0 = posición central.
     * +1000 = giro máximo hacia el otro lado.
     *
     * El bote utilizará esta orden para variar las revoluciones
     * entre el motor de babor y el motor de estribor.
     */
    /*
     * Vigente: demanda de giro entera de -100 a +100.
     *
     * -100 = giro maximo a la izquierda.
     *    0 = posicion central.
     * +100 = giro maximo a la derecha.
     *
     * No es un angulo de timon ni un angulo de camara.
     * En modo manual independiente se envia en cero.
     *
     * La mezcla ya se calcula en tierra; el receptor no debe
     * mezclar nuevamente las potencias utilizando este campo.
     */
    int16_t direccion;

    /*
     * Potencia solicitada para el motor de babor.
     *
     * Se almacena multiplicada por 10.
     *
     * Rango:
     *
     * 0     = 0.0 %
     * 1000  = 100.0 %
     */
    /*
     * Vigente: potencia final en porcentaje entero de 0 a 100.
     *
     * Manual: procede del potenciometro independiente de babor.
     * Joystick: procede del calculo con general y direccion.
     *
     * En joystick no debe superar potencia_global.
     * Es una orden de potencia, no una medicion de RPM.
     */
    uint16_t potencia_babor;

    /*
     * Potencia solicitada para el motor de estribor.
     *
     * Se almacena multiplicada por 10.
     *
     * Rango:
     *
     * 0     = 0.0 %
     * 1000  = 100.0 %
     */
    /*
     * Vigente: potencia final en porcentaje entero de 0 a 100.
     *
     * Manual: procede del potenciometro independiente de estribor.
     * Joystick: procede del calculo con general y direccion.
     *
     * En joystick no debe superar potencia_global.
     * Es una orden de potencia, no una medicion de RPM.
     */
    uint16_t potencia_estribor;

    /*
     * Ángulo solicitado para la cámara.
     *
     * Se almacena multiplicado por 10.
     *
     * Rango:
     *
     * -900 = -90.0 grados.
     *    0 = posición central.
     * +900 = +90.0 grados.
     */
    /*
     * El nombre y el rango anteriores se conservan para la memoria:
     * camara_x10 sigue almacenando decimas de grado, de -900 a +900.
     *
     * USV_ConstruirComando() convierte este valor a grados enteros
     * al transmitirlo. Por ejemplo, -871 se transmite como -87.
     *
     * USV_LeerComando() recibe grados enteros y los multiplica por 10
     * al guardarlos aqui. Por ejemplo, -45 se almacena como -450.
     *
     * Esto mantiene la compatibilidad con main.c y Nextion;
     * no significa que la trama siga enviando valores de -900 a +900.
     */
    /*
     * Vigente: grados enteros de -90 a +90 en memoria y por serial.
     * No existe camara_x10 ni se realizan conversiones por diez.
     *
     * Ejemplo: -45 se guarda como -45 y se transmite como -45.
     * La camara funciona en ambos modos de propulsion.
     */
    int16_t camara;

    /*
     * Orden digital para mover el motor de babor hacia avante.
     *
     * 0 = desactivado.
     * 1 = activado.
     */
    uint8_t babor_avante;

    /*
     * Orden digital para mover el motor de babor hacia atrás.
     *
     * 0 = desactivado.
     * 1 = activado.
     */
    uint8_t babor_atras;

    /*
     * Orden digital para mover el motor de estribor hacia avante.
     *
     * 0 = desactivado.
     * 1 = activado.
     */
    uint8_t estribor_avante;

    /*
     * Orden digital para mover el motor de estribor hacia atrás.
     *
     * 0 = desactivado.
     * 1 = activado.
     */
    uint8_t estribor_atras;

    /*
     * Estado solicitado para las luces de navegación.
     *
     * 0 = luces apagadas.
     * 1 = luces encendidas.
     */
    uint8_t luces;

    /*
     * Estado solicitado para la bomba de achique.
     *
     * 0 = bomba apagada.
     * 1 = bomba encendida.
     */
    uint8_t bomba;

    /*
     * Solicitud de reconexión del enlace con el bote.
     *
     * 0 = operación normal.
     * 1 = solicitar reconexión.
     */
    uint8_t reconexion;

    /*
     * Selección del modo de control.
     *
     * 0 = modo diferente al manual por joystick.
     * 1 = modo manual mediante joystick.
     */
    /*
     * Nombre y significado vigentes:
     *
     * modo_joystick = 0: manual con potenciometros independientes.
     * modo_joystick = 1: joystick con potenciometro general.
     *
     * Sustituye al miembro anterior llamado modo_manual.
     * Conserva la posicion numero 14 dentro de la trama.
     */
    uint8_t modo_joystick;

    /*
     * Estado del botón de parada de emergencia.
     *
     * 0 = operación normal.
     * 1 = parada de emergencia activa.
     *
     * Cuando este campo vale 1, el bote debe llevar
     * las órdenes de movimiento a una condición segura.
     */
    uint8_t parada;

    /*
     * Indica que se detectó una combinación contradictoria
     * en las entradas de dirección.
     *
     * Ejemplos:
     *
     * - Babor avante y babor atrás activos simultáneamente.
     * - Estribor avante y estribor atrás activos simultáneamente.
     *
     * 0 = combinación válida.
     * 1 = falla de dirección detectada.
     */
    uint8_t falla_direccion;

} USV_Comando;


/*
 * Estructura que contiene la telemetría enviada
 * desde el bote hacia el control de tierra.
 *
 * Su organización sigue la Trama Versión 3.
 */
/*
 * La telemetria no cambia con esta actualizacion del comando.
 * Sus miembros decimales conservan intencionalmente la escala x10.
 */
typedef struct
{
    /*
     * Hora UTC recibida del sistema GPS.
     *
     * Formato:
     *
     * hhmmss.ss
     */
    char utc[USV_TAMANO_UTC];

    /*
     * Latitud recibida del sistema GPS.
     *
     * Se conserva como texto para mantener la precisión.
     */
    char latitud[USV_TAMANO_COORDENADA];

    /*
     * Hemisferio correspondiente a la latitud.
     *
     * Valores esperados:
     *
     * N = Norte.
     * S = Sur.
     */
    char hemisferio_latitud;

    /*
     * Longitud recibida del sistema GPS.
     *
     * Se conserva como texto para mantener la precisión.
     */
    char longitud[USV_TAMANO_COORDENADA];

    /*
     * Hemisferio correspondiente a la longitud.
     *
     * Valores esperados:
     *
     * E = Este.
     * W = Oeste.
     */
    char hemisferio_longitud;

    /*
     * Calidad de la solución GPS.
     *
     * Rango admitido:
     *
     * 0 a 8.
     */
    uint8_t calidad_gps;

    /*
     * Cantidad de satélites utilizados por el GPS.
     *
     * Rango admitido:
     *
     * 0 a 24 satélites.
     */
    uint8_t satelites;

    /*
     * Dilución horizontal de precisión del GPS.
     *
     * Se almacena multiplicada por 10.
     *
     * Ejemplos:
     *
     * 5   = 0.5
     * 12  = 1.2
     * 500 = 50.0
     */
    uint16_t hdop_x10;

    /*
     * Altitud del bote en metros.
     *
     * Se almacena multiplicada por 10.
     *
     * Ejemplos:
     *
     * -100  = -10.0 metros.
     *  125  = 12.5 metros.
     */
    int32_t altitud_x10;

    /*
     * Velocidad sobre el terreno del bote.
     *
     * Se almacena multiplicada por 10.
     *
     * Rango definido por la Trama Versión 3:
     *
     * 0 a 30.0 nudos.
     */
    uint16_t velocidad_x10;

    /*
     * Rumbo sobre el terreno entregado por el GPS.
     *
     * Se almacena multiplicado por 10.
     *
     * Rango:
     *
     * 0 a 3599 = 0.0 a 359.9 grados.
     */
    uint16_t rumbo_x10;

    /*
     * Ángulo yaw o azimut entregado por la IMU.
     *
     * Se almacena multiplicado por 10.
     *
     * Rango:
     *
     * 0 a 3599 = 0.0 a 359.9 grados.
     */
    uint16_t yaw_x10;

    /*
     * Ángulo pitch entregado por la IMU.
     *
     * Se almacena multiplicado por 10.
     *
     * Rango:
     *
     * -900 a +900 = -90.0 a +90.0 grados.
     */
    int16_t pitch_x10;

    /*
     * Ángulo roll entregado por la IMU.
     *
     * Se almacena multiplicado por 10.
     *
     * Rango:
     *
     * -900 a +900 = -90.0 a +90.0 grados.
     */
    int16_t roll_x10;

    /*
     * Temperatura medida en el bote.
     *
     * Se almacena multiplicada por 10.
     *
     * Rango:
     *
     * -100 a 850 = -10.0 a 85.0 grados Celsius.
     */
    int16_t temperatura_x10;

    /*
     * Estado del sensor de inundación.
     *
     * 0 = compartimiento seco.
     * 1 = presencia de agua.
     */
    uint8_t inundacion;

    /*
     * Voltaje medido en la alimentación del bote.
     *
     * Se almacena multiplicado por 10.
     *
     * Rango:
     *
     * 0 a 140 = 0.0 a 14.0 voltios.
     */
    uint16_t voltaje_x10;

    /*
     * Corriente consumida por el sistema del bote.
     *
     * Se almacena multiplicada por 10.
     *
     * Rango:
     *
     * 0 a 1000 = 0.0 a 100.0 amperios.
     */
    uint16_t corriente_x10;

    /*
     * Estado real de las luces de navegación del bote.
     *
     * 0 = apagadas.
     * 1 = encendidas.
     */
    uint8_t luces;

    /*
     * Modo solicitado o reportado por el sistema.
     *
     * Valores definidos por la Trama Versión 3:
     *
     * 0 = reposo.
     * 1 = remoto.
     * 2 = reconectar.
     */
    uint8_t modo_solicitado;

} USV_Telemetria;


/*
 * Calcula el CRC-8 de un bloque de información.
 *
 * Se utiliza el polinomio:
 *
 * 0x07
 *
 * El cálculo se realiza sobre los caracteres ubicados
 * entre '$' y '*', sin incluir estos dos caracteres.
 *
 * Parámetros:
 *
 * datos    = dirección del bloque de datos.
 * longitud = cantidad de bytes que serán procesados.
 *
 * Retorna:
 *
 * Valor CRC-8 calculado.
 */
uint8_t USV_CRC8(
    const uint8_t *datos,
    size_t longitud);


/*
 * Verifica la estructura y el CRC de una trama recibida.
 *
 * Parámetro:
 *
 * trama = cadena de texto recibida por UART.
 *
 * Retorna:
 *
 * 1 = trama válida.
 * 0 = trama inválida.
 */
uint8_t USV_VerificarTrama(
    const char *trama);


/*
 * Construye la trama de comandos $PUSVU que será enviada
 * desde el control de tierra hacia el bote.
 *
 * Parámetros:
 *
 * destino         = buffer donde se guardará la trama.
 * tamano_destino  = tamaño total del buffer.
 * comando         = estructura con las órdenes del control.
 *
 * Retorna:
 *
 * Cantidad de caracteres escritos en el buffer.
 *
 * Retorna 0 si:
 *
 * - Algún puntero es inválido.
 * - El buffer es demasiado pequeño.
 * - Alguno de los valores está fuera de rango.
 */
/*
 * En esta version, la camara del comando se redondea al grado entero
 * mas cercano antes de construir la trama, sin utilizar float.
 * Los medios grados se redondean alejandose de cero:
 * -875 -> -88; -871 -> -87; 875 -> 88.
 * El campo transmitido queda siempre entre -90 y +90, sin decimales.
 */
/*
 * Vigente:
 *
 * Todos los miembros del comando ya contienen enteros
 * en las unidades que se transmiten.
 * No realiza conversiones x10 ni redondeos de la camara.
 *
 * Construye el identificador, los dieciseis valores,
 * el CRC-8 y los bytes finales CR/LF.
 *
 * La longitud retornada no incluye el caracter nulo '\0'.
 */
size_t USV_ConstruirComando(
    char *destino,
    size_t tamano_destino,
    const USV_Comando *comando);


/*
 * Verifica y decodifica una trama de comandos $PUSVU.
 *
 * Esta función se utilizará principalmente en el
 * microcontrolador instalado en el bote.
 *
 * Parámetros:
 *
 * trama   = cadena recibida mediante el XBee.
 * comando = estructura donde se guardarán las órdenes.
 *
 * Retorna:
 *
 * 1 = trama recibida y decodificada correctamente.
 * 0 = trama inválida, CRC incorrecto o valores fuera de rango.
 */
/*
 * El campo de camara recibido debe ser un entero entre -90 y +90.
 * No admite puntos decimales ni valores fuera de este rango.
 * Despues de validarlo, guarda grados * 10 en comando->camara_x10.
 * El receptor debe utilizar este mismo formato actualizado.
 */
/*
 * Vigente:
 *
 * Guarda los valores directamente, sin multiplicarlos por diez.
 *
 * - Potencias: 0 a 100.
 * - Direccion: -100 a +100.
 * - Camara: -90 a +90.
 * - Estados digitales: 0 o 1.
 *
 * Rechaza etiquetas, simbolos de porcentaje y puntos decimales
 * dentro de los campos numericos del comando.
 *
 * Si la trama es rechazada, no modifica la estructura destino.
 * Si existen ordenes simultaneas de avante y atras para un motor,
 * activa falla_direccion aunque el campo recibido fuera cero.
 *
 * Decodificar no acciona los motores ni el servomotor.
 * El receptor debe aplicar la seguridad antes de activar salidas
 * y llevar la propulsion a una condicion segura si pierde el enlace.
 */
uint8_t USV_LeerComando(
    const char *trama,
    USV_Comando *comando);


/*
 * Construye la trama de telemetría $PUSVD de acuerdo
 * con el formato definido en la Trama Versión 3.
 *
 * Esta función se utilizará principalmente en el
 * microcontrolador instalado en el bote.
 *
 * Parámetros:
 *
 * destino         = buffer donde se guardará la trama.
 * tamano_destino  = tamaño total del buffer.
 * telemetria      = estructura con los datos del bote.
 *
 * Retorna:
 *
 * Cantidad de caracteres escritos en el buffer.
 *
 * Retorna 0 si:
 *
 * - Algún puntero es inválido.
 * - El buffer es demasiado pequeño.
 * - Alguno de los datos está fuera de rango.
 */
size_t USV_ConstruirTelemetria(
    char *destino,
    size_t tamano_destino,
    const USV_Telemetria *telemetria);


/*
 * Verifica y decodifica una trama de telemetría $PUSVD.
 *
 * Esta función será utilizada por el control de tierra
 * para recuperar la información enviada desde el bote.
 *
 * Parámetros:
 *
 * trama      = cadena recibida mediante el XBee.
 * telemetria = estructura donde se guardarán los datos.
 *
 * Retorna:
 *
 * 1 = trama recibida y decodificada correctamente.
 * 0 = trama inválida, CRC incorrecto o valores fuera de rango.
 */
uint8_t USV_LeerTelemetria(
    const char *trama,
    USV_Telemetria *telemetria);


#endif /* TRAMA_USV_H_ */