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
#include <math.h>
#include "stm32h5xx_hal_gpio.h"
#include "string.h"
#include "stdlib.h"
#include "uart.h"


#include "gps.h"
#include "adc_x.h"
#include "temp_ds18b20.h"

#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

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
char texto[600];


volatile uint8_t imu_ok = 0U;
volatile uint8_t imu_addr = 0U;
volatile uint8_t imu_data_ok = 0U;

float imu_roll = 0.0f;
float imu_pitch = 0.0f;
float imu_yaw = 0.0f;

uint32_t imu_last_ms = 0U;
uint32_t teleplot_last_ms = 0U;
uint32_t temperatura_last_ms = 0U;
float temperatura_c = -100.0f;
uint8_t gps_rmc_ok = 0U;
uint8_t gps_gga_ok = 0U;
uint32_t gnss_rx_eventos = 0U;
uint32_t servo_test_last_ms = 0U;
uint8_t servo_test_estado = 0U;
float servo_test_angulo = 0.0f;
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

  /* PRUEBA DE VIDA DEL MICRO:
   * Se detiene aqui intencionalmente para verificar que el firmware
   * alcanza MX_GPIO_Init() y que PB2 puede conmutar.
   */
  while (1)
  {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(500U);
  }

  MX_GPDMA1_Init();
  MX_ICACHE_Init();
  MX_USART1_UART_Init();
  //MX_USART2_UART_Init();
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
SERVO_init(&SERVO1);
SERVO_ANG(&SERVO1, 0.0f); // Posiciona inicialmente la cámara al centro (0°)
uartx_write_text(&huart6, "INICIANDO\r\n");
//uartRX_it_idle_dma_init(&UARTRX1);
uartRX_it_idle_dma_init(&GPS_UARTRX);   // USART1 / estacion de tierra
uartRX_it_idle_dma_init(&GNSS_UARTRX);  // USART2 / GPS L76K

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

            /* Carga yaw, pitch y roll reales en la estructura $PUSVD. */
            USV_ActualizarTelemetriaIMU(
                imu_roll,
                imu_pitch,
                imu_yaw);
        }
    }

    /*
     * PRUEBA LOCAL SERVO MG996R:
     * Sin estacion de tierra. Usa exclusivamente la libreria servos.
     * Secuencia cada 3 s: -90 -> 0 -> +90 -> 0 grados.
     * No usa HAL_Delay para el movimiento del servo.
     */
    if ((HAL_GetTick() - servo_test_last_ms) >= 3000U)
    {
        servo_test_last_ms = HAL_GetTick();

        switch (servo_test_estado)
        {
            case 0U:
                servo_test_angulo = -90.0f;
                break;

            case 1U:
                servo_test_angulo = 0.0f;
                break;

            case 2U:
                servo_test_angulo = 90.0f;
                break;

            default:
                servo_test_angulo = 0.0f;
                break;
        }

        SERVO_ANG(&SERVO1, servo_test_angulo);

        servo_test_estado++;
        if (servo_test_estado > 3U)
        {
            servo_test_estado = 0U;
        }
    }

    /*
     * PRUEBA SENSOR DE TEMPERATURA DS18B20:
     * PC2 = TEMPE = bus 1-Wire.
     * La libreria realiza internamente la conversion y devuelve grados Celsius.
     * Se actualiza cada 2 s. TEMPE_Read() es bloqueante durante ~750 ms.
     */
    if ((HAL_GetTick() - temperatura_last_ms) >= 2000U)
    {
        temperatura_last_ms = HAL_GetTick();
        temperatura_c = TEMPE_Read();
    }

    /*
     * PRUEBA SENSOR DE HUMEDAD:
     * PC3 = ADC1_INP13 = Rank 3 = adc1_codigo[2].
     *
     * Calibracion experimental:
     *   ADC = 4095 -> 0 % mojado (seco)
     *   ADC = 1466 -> 100 % mojado
     *
     * La conversion se realiza aqui en main.c, sin modificar adc_x.
     */
    if ((HAL_GetTick() - teleplot_last_ms) >= 200U)
    {
        float humedad_pct;

        teleplot_last_ms = HAL_GetTick();

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

        sprintf(
            texto,
            ">humedad_adc:%u\r\n"
            ">humedad_pct:%.1f\r\n"
            ">temperatura_c:%.2f\r\n"
            ">servo_angulo:%.1f\r\n"
            ">servo_pwm_us:%lu\r\n"
            ">imu_ok:%u\r\n"
            ">imu_addr:%u\r\n"
            ">imu_data_ok:%u\r\n"
            ">roll:%.2f\r\n"
            ">pitch:%.2f\r\n"
            ">yaw:%.2f\r\n"
            ">gnss_rx_eventos:%lu\r\n"
            ">gnss_rx_bytes:%u\r\n"
            ">gps_rmc_ok:%u\r\n"
            ">gps_gga_ok:%u\r\n"
            ">gps_fix:%d\r\n"
            ">gps_satelites:%d\r\n"
            ">gps_hdop:%.2f\r\n"
            ">gps_latitud:%.6f\r\n"
            ">gps_longitud:%.6f\r\n"
            ">gps_altitud_m:%.2f\r\n"
            ">gps_velocidad_kph:%.2f\r\n"
            ">gps_rumbo:%.2f\r\n",
            (unsigned int)adc1_codigo[2],
            humedad_pct,
            temperatura_c,
            servo_test_angulo,
            (unsigned long)TIM3->CCR3,
            (unsigned int)imu_ok,
            (unsigned int)imu_addr,
            (unsigned int)imu_data_ok,
            imu_roll,
            imu_pitch,
            imu_yaw,
            (unsigned long)gnss_rx_eventos,
            (unsigned int)GNSS_UARTRX.num_datos,
            (unsigned int)gps_rmc_ok,
            (unsigned int)gps_gga_ok,
            (int)gps_modo,
            (int)gps_satelites,
            gps_hor_dilu,
            latitud,
            longitud,
            gps_altura,
            gps_vel_kph,
            gps_rumbo);

        uartx_write_text(&huart6, texto);
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

    // Verifica si USART1 recibió una trama desde Tierra mediante ReceiveToIdle por interrupción
    if (GPS_UARTRX.flag_rx == 1)
    {
      PUSVU();
	  	  		 sprintf(texto,"%.1f\r\n",ANG_SERVO);
	  		 uartx_write_text(&huart6,texto);
  
      
      /*
         * Cada recepción actualiza la marca de tiempo del enlace.
         * El parpadeo de PB2 se ejecuta abajo, sin bloquear el while.
         */
       // led_rx_ultimo_evento_ms = HAL_GetTick();
       // led_rx_activo = 1U;
HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
       //procesa_rx();                 // Decodifica $PUSVU y distribuye las ordenes del control de tierra
        uartRX_DMA_Re_init(&GPS_UARTRX); // Reinicia ReceiveToIdle por interrupción para la siguiente trama
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

        uartRX_DMA_Re_init(&GNSS_UARTRX);
    }

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

    /* Envia $PUSVD hacia tierra cada 500 ms. */
    TELEMETRIA_USV_Tarea(&TELEMETRIA1);

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
    //HAL_NVIC_SetPriority(GPDMA1_Channel2_IRQn, 0, 0);
    //HAL_NVIC_EnableIRQ(GPDMA1_Channel2_IRQn);

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
                          |DO4_PB15_Pin|ACHIQUE_CTRL_Pin, GPIO_PIN_RESET);

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
                           DO4_PB15_Pin ACHIQUE_CTRL_Pin */
  GPIO_InitStruct.Pin = LED_Pin|DO1_PB12_Pin|DO2_PB13_Pin|DO3_PB14_Pin
                          |DO4_PB15_Pin|ACHIQUE_CTRL_Pin;
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
