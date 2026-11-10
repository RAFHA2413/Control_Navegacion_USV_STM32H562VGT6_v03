/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "servos.h"
#include "UARTRX.h"
#include <stdio.h>
#include "imu_bno085_i2c.h"
#include "TELEMETRIA_USV.h"
#include "trama_usv.h"
#include <math.h>
#include "stm32h5xx_hal_gpio.h"
#include "string.h"
#include "stdlib.h"
#include "uart.h"


#include "gps.h"
#include "adc_x.h"
#include "temp_ds18b20.h"
#include "usv_motores.h"
#include "pcb_banco.h"
#include "corriente_lem.h"

#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*
 * PRUEBA 12.1:
 * La estacion puede ordenar y aplicar el rango completo -100..+100 %.
 * Se mantienen parada de emergencia y failsafe de enlace a 500 ms.
 */
#define USV_PRUEBA_MOTOR_LIMITE_PCT   100
#define USV_FAILSAFE_TIERRA_MS         500U
/* Tiempo inicial, ajustar con mediciones y perfiles reales XCTU. */
#define XBEE_REPLY_GUARD_MS             60U
/* Rearme RX solamente; no prolongar el failsafe de la propulsion. */
#define XBEE_RX_REARME_SILENCIO_MS     3000U
#define XBEE_RX_REARME_INTERVALO_MS   1000U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_NodeTypeDef Node_GPDMA1_Channel1;
DMA_QListTypeDef List_GPDMA1_Channel1;
DMA_HandleTypeDef handle_GPDMA1_Channel1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;
DMA_HandleTypeDef handle_GPDMA1_Channel0;
DMA_HandleTypeDef handle_GPDMA1_Channel2;

/* USER CODE BEGIN PV */
extern volatile uint32_t usv_xbee_uart1_error_irq;
extern volatile uint32_t usv_xbee_uart1_error_codigo;
extern volatile uint8_t usv_xbee_uart1_recuperar;
char texto[600];


volatile uint8_t imu_ok = 0U;
volatile uint8_t imu_addr = 0U;
volatile uint8_t imu_data_ok = 0U;

float imu_roll = 0.0f;
float imu_pitch = 0.0f;
float imu_yaw = 0.0f;

uint32_t imu_last_ms = 0U;
uint32_t teleplot_last_ms = 0U;

/* Heartbeat del lazo principal: incrementa una vez por segundo. */
uint32_t main_alive_last_ms = 0U;
uint32_t main_alive = 0U;

uint32_t temperatura_last_ms = 0U;
uint32_t temperatura_conversion_inicio_ms = 0U;
uint8_t temperatura_conversion_activa = 0U;
float temperatura_c = -100.0f;
uint8_t gps_rmc_ok = 0U;
uint8_t gps_gga_ok = 0U;
uint32_t gnss_rx_eventos = 0U;
uint32_t gnss_ultimo_fix_ms = 0U;
uint32_t servo_test_last_ms = 0U;
uint8_t servo_test_estado = 0U;
float servo_test_angulo = 0.0f;
uint16_t motor_babor_pwm_us = PWM_NEUTRO;
uint16_t motor_estribor_pwm_us = PWM_NEUTRO;
uint8_t motores_pwm_activos = 0U;
uint32_t motores_test_last_ms = 0U;
uint8_t motores_test_estado = 0U;

/* Prueba local de bomba de achique por PB5 / ACHIQUE_CTRL. */
uint32_t bomba_test_last_ms = 0U;
uint8_t bomba_test_estado = 0U;

uint16_t corriente_adc_raw = 0U;
float corriente_voltaje_v = 0.0f;
float corriente_lem_a = 0.0f;

uint16_t bateria_adc_raw = 0U;
float bateria_adc_v = 0.0f;
float bateria_voltaje_v = 0.0f;

/* Ultima trama $PUSVU decodificada desde USART1 / estacion de tierra. */
USV_Comando comando_tierra;
uint8_t comando_tierra_valido = 0U;
static uint8_t xbee_fragmento_rx[600U];
static char xbee_linea[USV_TRAMA_MAXIMA];
static uint16_t xbee_linea_n = 0U;
static uint32_t xbee_tramas_descartadas = 0U;
static uint32_t xbee_rx_eventos = 0U;
static uint32_t xbee_rx_bytes = 0U;
static uint32_t xbee_rx_ultimo_evento_ms = 0U;
static uint32_t xbee_rx_ultimo_rearme_ms = 0U;
static uint32_t xbee_rx_rearmes_ok = 0U;
static uint32_t xbee_rx_rearmes_error = 0U;
static uint32_t xbee_rx_watchdog = 0U;
static uint8_t xbee_reply_pendiente = 0U;
static uint32_t xbee_reply_deadline_ms = 0U;

/* Estado de control remoto para la prueba 12.0. */
int16_t cmd_babor_rx_pct = 0;
int16_t cmd_estribor_rx_pct = 0;
int16_t cmd_camara_rx_deg = 0;
int16_t motor_babor_aplicado_pct = 0;
int16_t motor_estribor_aplicado_pct = 0;
uint8_t parada_rx = 0U;
uint8_t enlace_tierra_ok = 0U;
uint8_t failsafe_tierra_activo = 1U;
uint32_t comando_tierra_ultimo_valido_ms = 0U;
uint32_t pusvu_rx_validas = 0U;

/* Diagnostico desacoplado de la recepcion para no bloquear USART1. */
uint32_t pusvu_diag_last_ms = 0U;
uint8_t pusvu_diag_pendiente = 0U;
uint8_t pusvu_diag_ultimo_valido = 0U;

char teleplot_tx[240];
uint8_t imu_raw[23];

int16_t imu_qi_raw = 0;
int16_t imu_qj_raw = 0;
int16_t imu_qk_raw = 0;
int16_t imu_qr_raw = 0;

volatile uint8_t imu_raw_ok = 0U;
volatile uint8_t imu_raw_channel = 0U;
volatile uint8_t imu_raw_report = 0U;
volatile uint16_t imu_raw_length = 0U;

/*
 * Diagnostico visual del enlace ESTACION -> BOTE.
 *
 * Mientras lleguen tramas por USART1, PB2 conmuta cada 250 ms.
 * Si pasan mas de 400 ms sin recibir nada, se detiene el parpadeo.
 */
uint32_t led_rx_ultimo_evento_ms = 0U;
uint32_t led_rx_ultimo_toggle_ms = 0U;
uint8_t led_rx_activo = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_ICACHE_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */
void procesa_rx(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* DMA+IDLE no representa trama completa: acumular fragmentos hasta CRLF. */
static uint8_t XBee_ExtraerComandos(
        const uint8_t *bytes, uint16_t cantidad,
        USV_Comando *comando, uint8_t *linea_completa)
{
    uint8_t correcto = 0U;
    uint16_t i;
    if ((bytes == NULL) || (comando == NULL) || (linea_completa == NULL))
        return 0U;
    *linea_completa = 0U;
    for (i = 0U; i < cantidad; ++i)
    {
        uint8_t c = bytes[i];
        if (c == (uint8_t)'$')
            xbee_linea_n = 0U;
        else if (xbee_linea_n == 0U)
            continue;
        if (xbee_linea_n >= USV_TRAMA_MAXIMA - 1U)
        {
            ++xbee_tramas_descartadas;
            xbee_linea_n = 0U;
            continue;
        }
        xbee_linea[xbee_linea_n++] = (char)c;
        if (c == (uint8_t)'\n')
        {
            USV_Comando actual;
            xbee_linea[xbee_linea_n] = '\0';
            *linea_completa = 1U;
            if (USV_LeerComando(xbee_linea, &actual) != 0U)
            {
                *comando = actual;
                correcto = 1U;
            }
            else
                ++xbee_tramas_descartadas;
            xbee_linea_n = 0U;
        }
    }
    return correcto;
}



/* Convierte coordenadas decimales entregadas por gps.c al formato NMEA
 * ddmm.mmmm / dddmm.mmmm esperado por Trama Version 3. */
static void USV_GNSS_FormatearCoord(double decimal, uint8_t lon,
                                   char *destino, size_t capacidad)
{
    double abs_coord = (decimal < 0.0) ? -decimal : decimal;
    unsigned long grados = (unsigned long)abs_coord;
    double minutos = (abs_coord - (double)grados) * 60.0;
    if (minutos >= 59.99995)
    {
        ++grados;
        minutos = 0.0;
    }
    if (lon != 0U)
        (void)snprintf(destino, capacidad, "%03lu%07.4f", grados, minutos);
    else
        (void)snprintf(destino, capacidad, "%02lu%07.4f", grados, minutos);
}

/* Sincroniza la estructura de telemetria con las mediciones GNSS decodificadas.
 * NO cambia la velocidad y el rumbo si aun no hay solucion valida. */
static void USV_ActualizarTelemetriaGNSS(uint8_t fix_valido)
{
    char utc[USV_TAMANO_UTC];
    char lat_nmea[USV_TAMANO_COORDENADA];
    char lon_nmea[USV_TAMANO_COORDENADA];
    uint16_t hdop10, vel10, rumbo10;
    int32_t alt10;
    uint8_t calidad, sats;
    uint8_t hh = (uint8_t)(((int16_t)hor_gps + 5 + 24) % 24);

    if (fix_valido == 0U)
    {
        TELEMETRIA1.datos.calidad_gps = 0U;
        TELEMETRIA1.datos.satelites = 0U;
        return;
    }
    /* gps.c ofrece hora LOCAL de Colombia; convertir de nuevo a UTC. */
    (void)snprintf(utc, sizeof(utc), "%02u%02u%02u.00",
                   (unsigned)hh, (unsigned)min_gps, (unsigned)seg_gps);
    USV_GNSS_FormatearCoord(latitud, 0U, lat_nmea, sizeof(lat_nmea));
    USV_GNSS_FormatearCoord(longitud, 1U, lon_nmea, sizeof(lon_nmea));
    calidad = (uint8_t)((gps_modo >= 1 && gps_modo <= 8) ?
                         gps_modo : 1);
    sats = (uint8_t)((gps_satelites >= 0 && gps_satelites <= 24) ?
                     gps_satelites : 0);
    hdop10 = (uint16_t)((gps_hor_dilu >= 0.5f &&
                         gps_hor_dilu <= 50.0f) ?
                        (gps_hor_dilu * 10.0f + 0.5f) : 500.0f);
    alt10 = (int32_t)((gps_altura >= -10.0f &&
                       gps_altura <= 9999.0f) ?
                       (gps_altura * 10.0f) : 0.0f);
    /* La Trama V3 etiqueta velocidad con N: transmitir NUDOS, no km/h. */
    vel10 = (uint16_t)((gps_vel_nudos >= 0.0f &&
                        gps_vel_nudos <= 30.0f) ?
                        (gps_vel_nudos * 10.0f + 0.5f) : 0.0f);
    rumbo10 = (uint16_t)((gps_rumbo >= 0.0f &&
                          gps_rumbo < 360.0f) ?
                         (gps_rumbo * 10.0f + 0.5f) : 0.0f);
    TELEMETRIA_USV_GPS(&TELEMETRIA1, utc, lat_nmea,
                       (latitud < 0.0) ? 'S' : 'N',
                       lon_nmea, (longitud < 0.0) ? 'W' : 'E',
                       calidad, sats, hdop10, alt10, vel10, rumbo10);
}

/*
 * Convierte los angulos reales del BNO085 al formato exigido por $PUSVD.
 *
 * - yaw:   0.0 ... 359.9 grados, almacenado x10.
 * - pitch: -90.0 ... +90.0 grados, almacenado x10.
 * - roll:  -90.0 ... +90.0 grados, almacenado x10.
 *
 * El BNO085 puede entregar roll fuera de +/-90 dependiendo de la orientacion.
 * La Trama Version 3 no admite ese rango, por eso se satura antes de enviarlo.
 */
static void USV_ActualizarTelemetriaIMU(
        float roll,
        float pitch,
        float yaw)
{
    int32_t yaw_x10;
    int32_t pitch_x10;
    int32_t roll_x10;

    while (yaw < 0.0f)
    {
        yaw += 360.0f;
    }

    while (yaw >= 360.0f)
    {
        yaw -= 360.0f;
    }

    if (pitch < -90.0f)
    {
        pitch = -90.0f;
    }
    else if (pitch > 90.0f)
    {
        pitch = 90.0f;
    }

    if (roll < -90.0f)
    {
        roll = -90.0f;
    }
    else if (roll > 90.0f)
    {
        roll = 90.0f;
    }

    yaw_x10 =
        (int32_t)((yaw * 10.0f) + 0.5f);

    pitch_x10 =
        (int32_t)((pitch >= 0.0f) ?
            ((pitch * 10.0f) + 0.5f) :
            ((pitch * 10.0f) - 0.5f));

    roll_x10 =
        (int32_t)((roll >= 0.0f) ?
            ((roll * 10.0f) + 0.5f) :
            ((roll * 10.0f) - 0.5f));

    if (yaw_x10 > 3599)
    {
        yaw_x10 = 3599;
    }

    TELEMETRIA_USV_IMU(
        &TELEMETRIA1,
        (uint16_t)yaw_x10,
        (int16_t)pitch_x10,
        (int16_t)roll_x10);
}

/*
 * Reconstruye el porcentaje con signo desde la estructura decodificada.
 * Positivo = avante, negativo = reversa.
 */
static int16_t USV_ComandoMotorFirmado(
        uint16_t potencia,
        uint8_t avante,
        uint8_t atras)
{
    if ((avante != 0U) && (atras == 0U))
    {
        return (int16_t)potencia;
    }

    if ((avante == 0U) && (atras != 0U))
    {
        return -(int16_t)potencia;
    }

    return 0;
}

/*
 * Limita el porcentaje de prueba y lo convierte a PWM ESC:
 * -100 % = 1000 us, 0 % = 1500 us, +100 % = 2000 us.
 * Para la prueba 12.1 se habilita el rango completo +/-100 %.
 */
static uint16_t USV_MotorPctAPwmPrueba(
        int16_t porcentaje,
        int16_t *porcentaje_aplicado)
{
    int16_t limitado = porcentaje;
    int32_t pwm;

    if (limitado > USV_PRUEBA_MOTOR_LIMITE_PCT)
    {
        limitado = USV_PRUEBA_MOTOR_LIMITE_PCT;
    }
    else if (limitado < -USV_PRUEBA_MOTOR_LIMITE_PCT)
    {
        limitado = -USV_PRUEBA_MOTOR_LIMITE_PCT;
    }

    if (porcentaje_aplicado != NULL)
    {
        *porcentaje_aplicado = limitado;
    }

    pwm = (int32_t)PWM_NEUTRO + ((int32_t)limitado * 5);

    if (pwm > PWM_MAX_ADELANTE)
    {
        pwm = PWM_MAX_ADELANTE;
    }
    else if (pwm < PWM_MAX_REVERSA)
    {
        pwm = PWM_MAX_REVERSA;
    }

    return (uint16_t)pwm;
}

/* void procesa_rx()
 {
  char procesa[100];
    char texto[100];
if (strstr(UARTRX1.trama_rx,"$PUSVU"))
	    {
	        strcpy(procesa, strtok(UARTRX1.trama_rx, "$"));  //inicia captura de tokens desde el =
	        strcpy(procesa, strtok(0, ","));  //captura hasta el /
strcpy(procesa, strtok(0, ","));  //captura hasta el /
strcpy(procesa, strtok(0, ","));  //captura hasta el /
strcpy(procesa, strtok(0, ","));  //captura hasta el /
strcpy(procesa, strtok(0, ","));  //captura hasta el /
strcpy(procesa, strtok(0, ","));  //captura hasta el /
 float serv=atof(procesa); // con signo
 sprintf(texto, "Servo= %.2f\r\n\r\n", serv);
 uartx_write_text(&huart1, texto);

 } 
  } */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_ICACHE_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART6_UART_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

/*
 * AUTOPRUEBA PB2:
 * Al arrancar, el LED cambia de estado durante 300 ms y vuelve al estado inicial.
 * Si este destello no se ve, el problema esta en PB2/LED y no en la UART.
 */
HAL_GPIO_WritePin(
    LED_RX_TIERRA_GPIO_Port,
    LED_RX_TIERRA_Pin,
    GPIO_PIN_SET);
HAL_Delay(300U);

HAL_GPIO_WritePin(
    LED_RX_TIERRA_GPIO_Port,
    LED_RX_TIERRA_Pin,
    GPIO_PIN_RESET);
HAL_Delay(300U);

// Inicializa la señal PWM en el pin PB0 (SERVO_CAMARA)
#if !PCB_BANCO_ACTIVO
SERVO_init(&SERVO1);
SERVO_ANG(&SERVO1, 0.0f);
#else
/* En banco, no generar PWM del servo hasta recibir orden manual. */
servo_test_angulo = 0.0f;
#endif // Posiciona inicialmente la cámara al centro (0°)

/*
 * PRUEBA LOCAL MOTORES 6.2:
 * Inicia únicamente las salidas PWM de propulsion en NEUTRO.
 * No se ejecuta USV_Motor_Calibrar() y no se ordena movimiento.
 * Babor    = PB4 / TIM3_CH1
 * Estribor = PB1 / TIM3_CH4
 */
if ((HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) == HAL_OK) &&
    (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4) == HAL_OK))
{
    USV_Motor_Set(&htim3, TIM_CHANNEL_1, PWM_NEUTRO);
    USV_Motor_Set(&htim3, TIM_CHANNEL_4, PWM_NEUTRO);

    motor_babor_pwm_us = PWM_NEUTRO;
    motor_estribor_pwm_us = PWM_NEUTRO;
    motores_pwm_activos = 1U;
}

/*
 * La prueba de bomba inicia con el rele desenergizado.
 * PB5 es open-drain y el rele es activo en LOW:
 * SET = alta impedancia = OFF.
 * RESET = 0 V = ON.
 */
HAL_GPIO_WritePin(
    ACHIQUE_CTRL_GPIO_Port,
    ACHIQUE_CTRL_Pin,
    GPIO_PIN_SET);
bomba_test_estado = 0U;
bomba_test_last_ms = HAL_GetTick();

uartx_write_text(&huart6, "INICIANDO\r\n");
#if PCB_BANCO_ACTIVO
PCB_Banco_Iniciar();
#endif
//uartRX_it_idle_dma_init(&UARTRX1);
uartRX_it_idle_dma_init(&GPS_UARTRX);   // USART1 / estacion de tierra
xbee_rx_ultimo_evento_ms = HAL_GetTick();
xbee_rx_ultimo_rearme_ms = HAL_GetTick();
uartRX_it_idle_dma_init(&GNSS_UARTRX);   // USART2 / GPS L76K

// Inicializa la telemetria oficial $PUSVD por USART1 / XBee
TELEMETRIA_USV_init(&TELEMETRIA1);

// Inicializa BNO085
imu_ok = IMU_Init();
imu_addr = IMU_GetAddress7bit();

// Inicializa DS18B20 / 1-Wire en PC2 (TEMPE)
TEMPE_Init();

/*
 * Inicia ADC1 por DMA usando la libreria adc_x.
 * CubeMX tiene 3 conversiones:
 * [0] PA0, [1] PC1, [2] PC3 (humedad).
 * PC2 ya no pertenece al ADC: se usa para DS18B20 / 1-Wire.
 * Se pasa 3 de forma explicita para no modificar la libreria adc_x.
 */
ADC_Read_DMA(&hadc1, 3U, adc1_codigo);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
#if PCB_BANCO_ACTIVO
    PCB_Banco_Tarea();
#endif
    /*
     * Heartbeat de diagnostico.
     * Si main_alive sigue aumentando, el while(1) continua ejecutandose.
     */
    if ((uint32_t)(HAL_GetTick() - main_alive_last_ms) >= 1000U)
    {
        main_alive_last_ms = HAL_GetTick();
        main_alive++;
    }

    /* Lectura IMU cada 50 ms */
    if ((HAL_GetTick() - imu_last_ms) >= 50U)
    {
        imu_last_ms = HAL_GetTick();

        if (IMU_ReadEuler(
                &imu_roll,
                &imu_pitch,
                &imu_yaw) != 0U)
        {
            imu_data_ok = 1U;

            USV_ActualizarTelemetriaIMU(
                imu_roll,
                imu_pitch,
                imu_yaw);
        }
    }

    /*
     * PRUEBA 12.0:
     * Servo y motores ya no se fuerzan aqui a una posicion fija.
     * Las salidas se actualizan exclusivamente cuando llega una $PUSVU
     * valida y por el failsafe de perdida de enlace definido mas abajo.
     */

    /* La bomba arranca desenergizada y solo cambia por una orden
     * PUSVU valida o cuando el failsafe corta el enlace. */

    /*
     * TEMPERATURA DS18B20 NO BLOQUEANTE:
     *
     * La conversion se inicia cada 2 s y el main continua ejecutandose.
     * Transcurridos 750 ms se lee el scratchpad. De esta forma USART1,
     * Teleplot, IMU y el resto de tareas siguen siendo atendidos durante
     * el tiempo de conversion del sensor.
     */
    {
        uint32_t ahora_temp = HAL_GetTick();

        if (temperatura_conversion_activa == 0U)
        {
            if ((uint32_t)(ahora_temp - temperatura_last_ms) >= 2000U)
            {
                temperatura_last_ms = ahora_temp;

                if (TEMPE_StartConversion() != 0U)
                {
                    temperatura_conversion_inicio_ms = ahora_temp;
                    temperatura_conversion_activa = 1U;
                }
                else
                {
                    temperatura_c = -100.0f;
                }
            }
        }
        else if ((uint32_t)(ahora_temp - temperatura_conversion_inicio_ms) >= 750U)
        {
            temperatura_c = TEMPE_ReadResult();
            temperatura_conversion_activa = 0U;
            /* No inventar temperatura cuando el DS18B20 no responde.
             * La version 3 admite -10.0..85.0 C. */
            if ((temperatura_c >= -10.0f) && (temperatura_c <= 85.0f))
            {
                TELEMETRIA1.datos.temperatura_x10 =
                    (int16_t)((temperatura_c * 10.0f) +
                        ((temperatura_c >= 0.0f) ? 0.5f : -0.5f));
            }
        }
    }

    /*
     * TELEPLOT SEGURO 11.4:
     * Se limita a 2 Hz para reducir la carga de USART6 durante la prueba
     * continua de comunicacion con la estacion de tierra.
     */
    if ((HAL_GetTick() - teleplot_last_ms) >= 500U)
    {
        float humedad_pct;
        int len;

        teleplot_last_ms = HAL_GetTick();

        /*
         * SENSOR DE CORRIENTE LEM HASS 200-S:
         * PA0 = ADC1_INP0 = adc1_codigo[0].
         * Se conserva intacta la libreria corriente_lem.
         */
        corriente_adc_raw = adc1_codigo[0];
        corriente_voltaje_v =
            ((float)corriente_adc_raw / STM32_ADC_MAX_CODES) *
            STM32_ADC_VREF;
        corriente_lem_a =
            LEM_HASS200_ObtenerCorriente(corriente_adc_raw);

        /*
         * SENSOR DE VOLTAJE DE BATERIA:
         * PC1 = ADC1_INP11 = adc1_codigo[1].
         * Divisor nominal: R1=20 kOhm, R2=4.7 kOhm.
         * Factor nominal = (20k + 4.7k) / 4.7k = 5.255.
         *
         * Para esta primera prueba se usa el factor nominal y se compara
         * contra el multimetro antes de aplicar cualquier calibracion.
         */
        bateria_adc_raw = adc1_codigo[1];
        bateria_adc_v =
            ((float)bateria_adc_raw / 4095.0f) * 3.3f;
        bateria_voltaje_v =
            bateria_adc_v * 5.255f;

        /*
         * INTEGRACION $PUSVD 9.1:
         * Convierte las mediciones electricas al formato x10 de la trama.
         * Se preservan temperatura e inundacion hasta integrarlas en su etapa.
         */
        {
            float bateria_tx_v = bateria_voltaje_v;
            float corriente_tx_a = corriente_lem_a;
            uint16_t bateria_tx_x10;
            uint16_t corriente_tx_x10;

            if (bateria_tx_v < 0.0f)
            {
                bateria_tx_v = 0.0f;
            }
            else if (bateria_tx_v > 14.0f)
            {
                bateria_tx_v = 14.0f;
            }

            if (corriente_tx_a < 0.0f)
            {
                corriente_tx_a = 0.0f;
            }
            else if (corriente_tx_a > 100.0f)
            {
                corriente_tx_a = 100.0f;
            }

            bateria_tx_x10 =
                (uint16_t)((bateria_tx_v * 10.0f) + 0.5f);

            corriente_tx_x10 =
                (uint16_t)((corriente_tx_a * 10.0f) + 0.5f);

            TELEMETRIA_USV_SENSORES(
                &TELEMETRIA1,
                TELEMETRIA1.datos.temperatura_x10,
                TELEMETRIA1.datos.inundacion,
                bateria_tx_x10,
                corriente_tx_x10);
        }

        humedad_pct =
            ((4095.0f - (float)adc1_codigo[2]) * 100.0f) /
            (4095.0f - 1466.0f);

        if (humedad_pct < 0.0f)
        {
            humedad_pct = 0.0f;
        }
        else if (humedad_pct > 100.0f)
        {
            humedad_pct = 100.0f;
        }

        /* Bloque 1: sensores basicos y servo. */
        len = snprintf(
            texto,
            sizeof(texto),
            ">humedad_adc:%u\r\n"
            ">humedad_pct:%.1f\r\n"
            ">temperatura_c:%.2f\r\n"
            ">servo_angulo:%.1f\r\n"
            ">servo_pwm_us:%lu\r\n"
            ">cmd_babor_rx_pct:%d\r\n"
            ">cmd_estribor_rx_pct:%d\r\n"
            ">cmd_camara_rx_deg:%d\r\n"
            ">motor_babor_aplicado_pct:%d\r\n"
            ">motor_estribor_aplicado_pct:%d\r\n"
            ">motores_pwm_activos:%u\r\n"
            ">motor_babor_pwm_us:%u\r\n"
            ">motor_estribor_pwm_us:%u\r\n"
            ">parada_rx:%u\r\n"
            ">enlace_tierra_ok:%u\r\n"
            ">failsafe_tierra:%u\r\n"
            ">pusvu_rx_validas:%lu\r\n"
            ">bomba_achique:%u\r\n"
            ">main_alive:%lu\r\n",
            (unsigned int)adc1_codigo[2],
            humedad_pct,
            temperatura_c,
            servo_test_angulo,
            (unsigned long)TIM3->CCR3,
            (int)cmd_babor_rx_pct,
            (int)cmd_estribor_rx_pct,
            (int)cmd_camara_rx_deg,
            (int)motor_babor_aplicado_pct,
            (int)motor_estribor_aplicado_pct,
            (unsigned int)motores_pwm_activos,
            (unsigned int)motor_babor_pwm_us,
            (unsigned int)motor_estribor_pwm_us,
            (unsigned int)parada_rx,
            (unsigned int)enlace_tierra_ok,
            (unsigned int)failsafe_tierra_activo,
            (unsigned long)pusvu_rx_validas,
            (unsigned int)bomba_test_estado,
            (unsigned long)main_alive);

        if ((len > 0) && ((size_t)len < sizeof(texto)))
        {
            uartx_write_text(&huart6, texto);
        }

        /* Bloque 2: diagnostico IMU. */
        len = snprintf(
            texto,
            sizeof(texto),
            ">imu_ok:%u\r\n"
            ">imu_addr:%u\r\n"
            ">imu_data_ok:%u\r\n"
            ">roll:%.2f\r\n"
            ">pitch:%.2f\r\n"
            ">yaw:%.2f\r\n",
            (unsigned int)imu_ok,
            (unsigned int)imu_addr,
            (unsigned int)imu_data_ok,
            imu_roll,
            imu_pitch,
            imu_yaw);

        if ((len > 0) && ((size_t)len < sizeof(texto)))
        {
            uartx_write_text(&huart6, texto);
        }

        /* Bloque 3: estado de recepcion GNSS. */
        len = snprintf(
            texto,
            sizeof(texto),
            ">gnss_rx_eventos:%lu\r\n"
            ">gnss_rx_bytes:%u\r\n"
            ">gps_rmc_ok:%u\r\n"
            ">gps_gga_ok:%u\r\n"
            ">gps_fix:%d\r\n"
            ">gps_satelites:%d\r\n"
            ">gps_hdop:%.2f\r\n",
            (unsigned long)gnss_rx_eventos,
            (unsigned int)GNSS_UARTRX.num_datos,
            (unsigned int)gps_rmc_ok,
            (unsigned int)gps_gga_ok,
            (int)gps_modo,
            (int)gps_satelites,
            gps_hor_dilu);

        if ((len > 0) && ((size_t)len < sizeof(texto)))
        {
            uartx_write_text(&huart6, texto);
        }

        /* Bloque 4: datos de navegacion GNSS. */
        len = snprintf(
            texto,
            sizeof(texto),
            ">gps_latitud:%.6f\r\n"
            ">gps_longitud:%.6f\r\n"
            ">gps_altitud_m:%.2f\r\n"
            ">gps_velocidad_kph:%.2f\r\n"
            ">gps_rumbo:%.2f\r\n",
            latitud,
            longitud,
            gps_altura,
            gps_vel_kph,
            gps_rumbo);

        if ((len > 0) && ((size_t)len < sizeof(texto)))
        {
            uartx_write_text(&huart6, texto);
        }

        /* Bloque 5: sensor de corriente LEM por PA0 / ADC1_INP0. */
        len = snprintf(
            texto,
            sizeof(texto),
            ">corriente_adc:%u\r\n"
            ">corriente_voltaje_v:%.3f\r\n"
            ">corriente_a:%.2f\r\n",
            (unsigned int)corriente_adc_raw,
            corriente_voltaje_v,
            corriente_lem_a);

        if ((len > 0) && ((size_t)len < sizeof(texto)))
        {
            uartx_write_text(&huart6, texto);
        }

        /* Bloque 6: voltaje de bateria por PC1 / ADC1_INP11. */
        len = snprintf(
            texto,
            sizeof(texto),
            ">bateria_adc:%u\r\n"
            ">bateria_adc_v:%.3f\r\n"
            ">bateria_voltaje_v:%.2f\r\n",
            (unsigned int)bateria_adc_raw,
            bateria_adc_v,
            bateria_voltaje_v);

        if ((len > 0) && ((size_t)len < sizeof(texto)))
        {
            uartx_write_text(&huart6, texto);
        }

        /* 14.1: indicadores locales de envio PUSVD por USART1/XBee.
         * tramas_enviadas solo confirma HAL_OK en UART, no recepcion RF. */
        len = snprintf(
            texto,
            sizeof(texto),
            ">pusvd_tx_uart_ok:%lu\r\n"
            ">pusvd_tx_uart_err:%lu\r\n"
            ">pusvd_tx_formato_err:%lu\r\n"
            ">pusvd_tx_ultimo_bytes:%u\r\n",
            (unsigned long)TELEMETRIA1.tramas_enviadas,
            (unsigned long)TELEMETRIA1.errores_uart,
            (unsigned long)TELEMETRIA1.errores_construccion,
            (unsigned int)TELEMETRIA1.longitud_tx);
        if ((len > 0) && ((size_t)len < sizeof(texto)))
        {
            uartx_write_text(&huart6, texto);
        }

        /* Diagnostico local: HAL_OK no implica entrega por RF. */
        len = snprintf(
            texto, sizeof(texto),
            ">xbee_rx_eventos:%lu\r\n"
            ">xbee_rx_bytes:%lu\r\n"
            ">xbee_rx_silencio_ms:%lu\r\n"
            ">xbee_rx_rearmes_ok:%lu\r\n"
            ">xbee_rx_rearmes_error:%lu\r\n"
            ">xbee_rx_watchdog:%lu\r\n"
            ">xbee_uart1_error_irq:%lu\r\n"
            ">xbee_uart1_error_codigo:%lu\r\n"
            ">xbee_rx_descartadas:%lu\r\n",
            (unsigned long)xbee_rx_eventos,
            (unsigned long)xbee_rx_bytes,
            (unsigned long)((uint32_t)(HAL_GetTick() - xbee_rx_ultimo_evento_ms)),
            (unsigned long)xbee_rx_rearmes_ok,
            (unsigned long)xbee_rx_rearmes_error,
            (unsigned long)xbee_rx_watchdog,
            (unsigned long)usv_xbee_uart1_error_irq,
            (unsigned long)usv_xbee_uart1_error_codigo,
            (unsigned long)xbee_tramas_descartadas);
        if ((len > 0) && ((size_t)len < sizeof(texto)))
        {
            uartx_write_text(&huart6, texto);
        }
#if PCB_BANCO_ACTIVO
        len = snprintf(texto, sizeof(texto),
            ">pcb_armado:%u\r\n>pcb_motor_pulso:%u\r\n"
            ">pcb_bomba_pulso:%u\r\n>pcb_do_pulso:%u\r\n"
            ">di1:%u\r\n>di2:%u\r\n>di3:%u\r\n>di4:%u\r\n"
            ">humedad_do:%u\r\n",
            (unsigned int)PCB_Banco_Armado(),
            (unsigned int)PCB_Banco_MotorActivo(),
            (unsigned int)PCB_Banco_BombaActiva(),
            (unsigned int)PCB_Banco_SalidaActiva(),
            (unsigned int)HAL_GPIO_ReadPin(DI1_PA4_GPIO_Port, DI1_PA4_Pin),
            (unsigned int)HAL_GPIO_ReadPin(DI2_PA5_GPIO_Port, DI2_PA5_Pin),
            (unsigned int)HAL_GPIO_ReadPin(DI3_PA6_GPIO_Port, DI3_PA6_Pin),
            (unsigned int)HAL_GPIO_ReadPin(DI4_PA7_GPIO_Port, DI4_PA7_Pin),
            (unsigned int)HAL_GPIO_ReadPin(HUMIDITY_DO_GPIO_Port, HUMIDITY_DO_Pin));
        if ((len > 0) && ((size_t)len < sizeof(texto)))
            uartx_write_text(&huart6, texto);
#endif
    }

    /* Envío a Teleplot cada 200 ms */
    /* if ((HAL_GetTick() - teleplot_last_ms) >= 200U)
    {
        teleplot_last_ms = HAL_GetTick();

        int len = snprintf(
            teleplot_tx,
            sizeof(teleplot_tx),
            ">imu_ok:%u\r\n"
            ">imu_addr:%u\r\n"
            ">imu_data_ok:%u\r\n"
            ">roll:%.2f\r\n"
            ">pitch:%.2f\r\n"
            ">yaw:%.2f\r\n"
            ">rx_eventos:%lu\r\n"
            ">tramas_validas:%lu\r\n"
            ">camara_rx:%d\r\n"
            ">servo_ccr3:%lu\r\n"
            ">pb2_estado:%u\r\n"
            ">enlace_tierra:%u\r\n",
            (unsigned int)imu_ok,
            (unsigned int)imu_addr,
            (unsigned int)imu_data_ok,
            imu_roll,
            imu_pitch,
            imu_yaw,
            (unsigned long)usv_rx_eventos,
            (unsigned long)usv_tramas_validas,
            (int)usv_camara_recibida,
            (unsigned long)TIM3->CCR3,
            (unsigned int)HAL_GPIO_ReadPin(
                LED_RX_TIERRA_GPIO_Port,
                LED_RX_TIERRA_Pin),
            (unsigned int)led_rx_activo);

        if (len > 0)
        {
            HAL_UART_Transmit(
                &huart6,
                (uint8_t *)teleplot_tx,
                (uint16_t)len,
                100U);
        }
    } */

    /*
     * PRUEBA 12.0 - CONTROL REAL DESDE $PUSVU:
     *
     * Una trama valida actualiza inmediatamente:
     * - servo PB0 / TIM3_CH3;
     * - motor babor PB4 / TIM3_CH1;
     * - motor estribor PB1 / TIM3_CH4.
     *
     * Los motores usan el rango completo -100..+100 %.
     * parada=1 mantiene ambos ESC en NEUTRO.
     */
    if (GPS_UARTRX.flag_rx == 1)
    {
        uint32_t ahora_rx = HAL_GetTick();

        xbee_rx_ultimo_evento_ms = ahora_rx;
        xbee_rx_eventos++;
        led_rx_ultimo_evento_ms = ahora_rx;
        led_rx_activo = 1U;

        uint16_t n_xbee = GPS_UARTRX.num_datos;
        xbee_rx_bytes += (uint32_t)n_xbee;
        uint8_t copia = (uint8_t)((GPS_UARTRX.trama_rx != NULL) &&
                                  (n_xbee > 0U) &&
                                  (n_xbee <= sizeof(xbee_fragmento_rx)));
        uint8_t linea_completa = 0U;
        if (copia != 0U)
            memcpy(xbee_fragmento_rx, GPS_UARTRX.trama_rx, n_xbee);
        else
            ++xbee_tramas_descartadas;
        /* Si HAL rechaza el rearme, watchdog reintentara desde main. */
        if (uartRX_DMA_Re_init(&GPS_UARTRX) == HAL_OK)
        {
            xbee_rx_rearmes_ok++;
            usv_xbee_uart1_recuperar = 0U;
        }
        else
        {
            xbee_rx_rearmes_error++;
            usv_xbee_uart1_recuperar = 1U;
        }
        comando_tierra_valido = copia
            ? XBee_ExtraerComandos(xbee_fragmento_rx, n_xbee,
                                    &comando_tierra, &linea_completa)
            : 0U;
        pusvu_diag_ultimo_valido = comando_tierra_valido;
        if (linea_completa != 0U)
            pusvu_diag_pendiente = 1U;

        if (comando_tierra_valido != 0U)
        {
            cmd_babor_rx_pct =
                USV_ComandoMotorFirmado(
                    comando_tierra.potencia_babor,
                    comando_tierra.babor_avante,
                    comando_tierra.babor_atras);

            cmd_estribor_rx_pct =
                USV_ComandoMotorFirmado(
                    comando_tierra.potencia_estribor,
                    comando_tierra.estribor_avante,
                    comando_tierra.estribor_atras);

            cmd_camara_rx_deg = comando_tierra.camara;
            parada_rx = comando_tierra.parada;

            comando_tierra_ultimo_valido_ms = ahora_rx;
            enlace_tierra_ok = 1U;
            failsafe_tierra_activo = 0U;
            pusvu_rx_validas++;
            /* Responder una sola vez por lote de comandos completos validos. */
            xbee_reply_pendiente = 1U;
            xbee_reply_deadline_ms = ahora_rx + XBEE_REPLY_GUARD_MS;

#if !PCB_BANCO_ACTIVO
            /* La camara sigue directamente el comando -90..+90 grados. */
            servo_test_angulo = (float)cmd_camara_rx_deg;
            SERVO_ANG(&SERVO1, servo_test_angulo);

            if ((parada_rx != 0U) || (motores_pwm_activos == 0U))
            {
                motor_babor_aplicado_pct = 0;
                motor_estribor_aplicado_pct = 0;
                motor_babor_pwm_us = PWM_NEUTRO;
                motor_estribor_pwm_us = PWM_NEUTRO;
            }
            else
            {
                motor_babor_pwm_us =
                    USV_MotorPctAPwmPrueba(
                        cmd_babor_rx_pct,
                        &motor_babor_aplicado_pct);

                motor_estribor_pwm_us =
                    USV_MotorPctAPwmPrueba(
                        cmd_estribor_rx_pct,
                        &motor_estribor_aplicado_pct);
            }

            if (motores_pwm_activos != 0U)
            {
                USV_Motor_Set(
                    &htim3,
                    TIM_CHANNEL_1,
                    motor_babor_pwm_us);

                USV_Motor_Set(
                    &htim3,
                    TIM_CHANNEL_4,
                    motor_estribor_pwm_us);
            }

            /* Relé de achique PB5 es activo en LOW; parada=1 siempre OFF.
             * En el arranque permanece OFF hasta la primera orden valida. */
            bomba_test_estado =
                (uint8_t)((comando_tierra.bomba != 0U) &&
                          (parada_rx == 0U));
            HAL_GPIO_WritePin(ACHIQUE_CTRL_GPIO_Port, ACHIQUE_CTRL_Pin,
                (bomba_test_estado != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
#endif
        }

    }

    /*
     * FAILSAFE DE ENLACE:
     * Si pasan mas de 500 ms sin una $PUSVU valida, ambos motores vuelven
     * a NEUTRO. El servo conserva la ultima posicion recibida.
     */
#if !PCB_BANCO_ACTIVO
    if ((enlace_tierra_ok != 0U) &&
        ((uint32_t)(HAL_GetTick() - comando_tierra_ultimo_valido_ms) >
         USV_FAILSAFE_TIERRA_MS))
    {
        enlace_tierra_ok = 0U;
        failsafe_tierra_activo = 1U;
        /* Parada de bomba ante perdida de enlace (rele activo en LOW). */
        bomba_test_estado = 0U;
        HAL_GPIO_WritePin(
            ACHIQUE_CTRL_GPIO_Port, ACHIQUE_CTRL_Pin, GPIO_PIN_SET);

        motor_babor_aplicado_pct = 0;
        motor_estribor_aplicado_pct = 0;
        motor_babor_pwm_us = PWM_NEUTRO;
        motor_estribor_pwm_us = PWM_NEUTRO;

        if (motores_pwm_activos != 0U)
        {
            USV_Motor_Set(
                &htim3,
                TIM_CHANNEL_1,
                motor_babor_pwm_us);

            USV_Motor_Set(
                &htim3,
                TIM_CHANNEL_4,
                motor_estribor_pwm_us);
        }
    }

#endif
    /*
     * Diagnostico $PUSVU por USART6 limitado a 500 ms.
     * Siempre muestra la ultima orden recibida.
     */
    if ((pusvu_diag_pendiente != 0U) &&
        ((uint32_t)(HAL_GetTick() - pusvu_diag_last_ms) >= 500U))
    {
        int len_rx;

        if (pusvu_diag_ultimo_valido != 0U)
        {
            int16_t babor_signed = 0;
            int16_t estribor_signed = 0;

            if (comando_tierra.babor_avante != 0U)
            {
                babor_signed =
                    (int16_t)comando_tierra.potencia_babor;
            }
            else if (comando_tierra.babor_atras != 0U)
            {
                babor_signed =
                    -(int16_t)comando_tierra.potencia_babor;
            }

            if (comando_tierra.estribor_avante != 0U)
            {
                estribor_signed =
                    (int16_t)comando_tierra.potencia_estribor;
            }
            else if (comando_tierra.estribor_atras != 0U)
            {
                estribor_signed =
                    -(int16_t)comando_tierra.potencia_estribor;
            }

            len_rx = snprintf(
                texto,
                sizeof(texto),
                "\r\n"
                "===== PUSVU DECODIFICADA =====\r\n"
                "PUSVU_OK          : 1\r\n"
                "Babor recibido    : %d %%\r\n"
                "Estribor recibido : %d %%\r\n"
                "Babor avante      : %u\r\n"
                "Babor atras       : %u\r\n"
                "Estribor avante   : %u\r\n"
                "Estribor atras    : %u\r\n"
                "Camara            : %d grados\r\n"
                "Bomba             : %u\r\n"
                "Parada emergencia : %u\r\n"
                "Babor aplicado    : %d %% / %u us\r\n"
                "Estribor aplicado : %d %% / %u us\r\n"
                "Failsafe enlace   : %u\r\n"
                "==============================\r\n",
                (int)babor_signed,
                (int)estribor_signed,
                (unsigned int)comando_tierra.babor_avante,
                (unsigned int)comando_tierra.babor_atras,
                (unsigned int)comando_tierra.estribor_avante,
                (unsigned int)comando_tierra.estribor_atras,
                (int)comando_tierra.camara,
                (unsigned int)comando_tierra.bomba,
                (unsigned int)comando_tierra.parada,
                (int)motor_babor_aplicado_pct,
                (unsigned int)motor_babor_pwm_us,
                (int)motor_estribor_aplicado_pct,
                (unsigned int)motor_estribor_pwm_us,
                (unsigned int)failsafe_tierra_activo);
        }
        else
        {
            len_rx = snprintf(
                texto,
                sizeof(texto),
                "\r\n"
                "===== PUSVU INVALIDA =====\r\n"
                "PUSVU_OK : 0\r\n"
                "==========================\r\n");
        }

        if ((len_rx > 0) && ((size_t)len_rx < sizeof(texto)))
        {
            uartx_write_text(&huart6, texto);
        }

        pusvu_diag_last_ms = HAL_GetTick();
        pusvu_diag_pendiente = 0U;
    }

    /*
     * GPS L76K / USART2:
     * La libreria gps.c busca RMC y GGA dentro del buffer NMEA recibido.
     * Los indicadores gps_rmc_ok / gps_gga_ok quedan enclavados cuando
     * se obtiene por primera vez una sentencia valida con solucion.
     */
    if (GNSS_UARTRX.flag_rx == 1)
    {
        uint8_t rmc_ok;
        uint8_t gga_ok;

        gnss_rx_eventos++;

        rmc_ok = GPS_RMC();
        gga_ok = GPS_GGA();

        if (rmc_ok != 0U)
        {
            gps_rmc_ok = 1U;
        }

        if (gga_ok != 0U)
        {
            gps_gga_ok = 1U;
        }
        if ((rmc_ok != 0U) || (gga_ok != 0U))
        {
            gnss_ultimo_fix_ms = HAL_GetTick();
            USV_ActualizarTelemetriaGNSS(1U);
        }
        uartRX_DMA_Re_init(&GNSS_UARTRX);
    }

    /* Un fix antiguo no puede seguir declarándose válido indefinidamente. */
    if ((uint32_t)(HAL_GetTick() - gnss_ultimo_fix_ms) > 3000U)
        TELEMETRIA1.datos.calidad_gps = 0U;

    /*
     * PRUEBA SOLICITADA POR EL PROFESOR:
     * PB2 parpadea aproximadamente a 2 Hz mientras se siguen recibiendo
     * tramas desde la estacion de tierra.
     */
    if (led_rx_activo != 0U)
    {
        uint32_t ahora_led = HAL_GetTick();

        if ((uint32_t)(ahora_led - led_rx_ultimo_evento_ms) > 400U)
        {
            /* No han llegado tramas recientemente: detiene el parpadeo. */
            led_rx_activo = 0U;
            HAL_GPIO_WritePin(
                LED_RX_TIERRA_GPIO_Port,
                LED_RX_TIERRA_Pin,
                GPIO_PIN_RESET);
        }
        else if ((uint32_t)(ahora_led - led_rx_ultimo_toggle_ms) >= 250U)
        {
            led_rx_ultimo_toggle_ms = ahora_led;

            HAL_GPIO_TogglePin(
                LED_RX_TIERRA_GPIO_Port,
                LED_RX_TIERRA_Pin);
        }
    }

    /* Rearme controlado de RX USART1 fuera de las interrupciones.
     * No toca PWM ni bomba: el failsafe permanece independiente. */
    {
        uint32_t ahora = HAL_GetTick();
        uint8_t sin_eventos =
            ((uint32_t)(ahora - xbee_rx_ultimo_evento_ms) >=
             XBEE_RX_REARME_SILENCIO_MS) ? 1U : 0U;
        if ((GPS_UARTRX.flag_rx == 0U) &&
            ((usv_xbee_uart1_recuperar != 0U) || (sin_eventos != 0U)) &&
            ((uint32_t)(ahora - xbee_rx_ultimo_rearme_ms) >=
             XBEE_RX_REARME_INTERVALO_MS))
        {
            HAL_StatusTypeDef estado;
            xbee_rx_ultimo_rearme_ms = ahora;
            xbee_rx_watchdog++;
            estado = HAL_UART_AbortReceive(&huart1);
            if (estado == HAL_OK)
            {
                estado = uartRX_DMA_Re_init(&GPS_UARTRX);
            }
            if (estado == HAL_OK)
            {
                xbee_rx_rearmes_ok++;
                usv_xbee_uart1_recuperar = 0U;
            }
            else
            {
                xbee_rx_rearmes_error++;
                usv_xbee_uart1_recuperar = 1U;
            }
        }
    }

    /* Half-duplex por RF: solo responder a un $PUSVU valido.
     * UART1 permanece con TX/RX separados: no usar HAL_HalfDuplex_Init. */
    if ((xbee_reply_pendiente != 0U) &&
        ((int32_t)(HAL_GetTick() - xbee_reply_deadline_ms) >= 0))
    {
        xbee_reply_pendiente = 0U;
        (void)TELEMETRIA_USV_Enviar(&TELEMETRIA1);
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the programming delay
  */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_1);
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 3;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.SamplingMode = ADC_SAMPLING_MODE_NORMAL;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_11;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_13;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief GPDMA1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPDMA1_Init(void)
{

  /* USER CODE BEGIN GPDMA1_Init 0 */

  /* USER CODE END GPDMA1_Init 0 */

  /* Peripheral clock enable */
  __HAL_RCC_GPDMA1_CLK_ENABLE();

  /* GPDMA1 interrupt Init */
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
    HAL_NVIC_SetPriority(GPDMA1_Channel2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel2_IRQn);

  /* USER CODE BEGIN GPDMA1_Init 1 */

  /* USER CODE END GPDMA1_Init 1 */
  /* USER CODE BEGIN GPDMA1_Init 2 */

  /* USER CODE END GPDMA1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00602173;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief ICACHE Initialization Function
  * @param None
  * @retval None
  */
static void MX_ICACHE_Init(void)
{

  /* USER CODE BEGIN ICACHE_Init 0 */

  /* USER CODE END ICACHE_Init 0 */

  /* USER CODE BEGIN ICACHE_Init 1 */

  /* USER CODE END ICACHE_Init 1 */

  /** Enable instruction cache in 1-way (direct mapped cache)
  */
  if (HAL_ICACHE_ConfigAssociativityMode(ICACHE_1WAY) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ICACHE_Init 2 */

  /* USER CODE END ICACHE_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 63;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 19999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.Pulse = 1500;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.Pulse = 0;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart6.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart6, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart6, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(TEMPE_GPIO_Port, TEMPE_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_Pin|DO1_PB12_Pin|DO2_PB13_Pin|DO3_PB14_Pin
                          |DO4_PB15_Pin, GPIO_PIN_RESET);

  /* PB5 open-drain: SET = alta impedancia = rele desactivado. */
  HAL_GPIO_WritePin(
      ACHIQUE_CTRL_GPIO_Port,
      ACHIQUE_CTRL_Pin,
      GPIO_PIN_SET);

  /*Configure GPIO pin : TEMPE_Pin */
  GPIO_InitStruct.Pin = TEMPE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TEMPE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DI1_PA4_Pin DI2_PA5_Pin DI3_PA6_Pin DI4_PA7_Pin */
  GPIO_InitStruct.Pin = DI1_PA4_Pin|DI2_PA5_Pin|DI3_PA6_Pin|DI4_PA7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : HUMIDITY_DO_Pin */
  GPIO_InitStruct.Pin = HUMIDITY_DO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(HUMIDITY_DO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_Pin DO1_PB12_Pin DO2_PB13_Pin DO3_PB14_Pin
                           DO4_PB15_Pin */
  GPIO_InitStruct.Pin = LED_Pin|DO1_PB12_Pin|DO2_PB13_Pin|DO3_PB14_Pin
                          |DO4_PB15_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF0_TRACE;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ACHIQUE_CTRL_Pin */
  GPIO_InitStruct.Pin = ACHIQUE_CTRL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ACHIQUE_CTRL_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /*
   * PB2: LED de prueba del enlace ESTACION -> BOTE.
   * Se configura aqui para no depender de una regeneracion de CubeMX.
   */
  HAL_GPIO_WritePin(
      LED_RX_TIERRA_GPIO_Port,
      LED_RX_TIERRA_Pin,
      GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = LED_RX_TIERRA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(
      LED_RX_TIERRA_GPIO_Port,
      &GPIO_InitStruct);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};
  MPU_Attributes_InitTypeDef MPU_AttributesInit = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region 0 and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x08FFF000;
  MPU_InitStruct.LimitAddress = 0x08FFFFFF;
  MPU_InitStruct.AttributesIndex = MPU_ATTRIBUTES_NUMBER0;
  MPU_InitStruct.AccessPermission = MPU_REGION_ALL_RO;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Attribute 0 and the memory to be protected
  */
  MPU_AttributesInit.Number = MPU_ATTRIBUTES_NUMBER0;
  MPU_AttributesInit.Attributes = INNER_OUTER(MPU_NOT_CACHEABLE);

  HAL_MPU_ConfigMemoryAttributes(&MPU_AttributesInit);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
