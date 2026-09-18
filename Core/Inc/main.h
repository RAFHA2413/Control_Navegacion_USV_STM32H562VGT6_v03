/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USART2_TX_GNSS_Pin GPIO_PIN_2
#define USART2_TX_GNSS_GPIO_Port GPIOA
#define USART2_RX_GNSS_Pin GPIO_PIN_3
#define USART2_RX_GNSS_GPIO_Port GPIOA
#define DI1_PA4_Pin GPIO_PIN_4
#define DI1_PA4_GPIO_Port GPIOA
#define DI2_PA5_Pin GPIO_PIN_5
#define DI2_PA5_GPIO_Port GPIOA
#define DI3_PA6_Pin GPIO_PIN_6
#define DI3_PA6_GPIO_Port GPIOA
#define DI4_PA7_Pin GPIO_PIN_7
#define DI4_PA7_GPIO_Port GPIOA
#define HUMIDITY_DO_Pin GPIO_PIN_4
#define HUMIDITY_DO_GPIO_Port GPIOC
#define SERVO_CAMARA_Pin GPIO_PIN_0
#define SERVO_CAMARA_GPIO_Port GPIOB
#define LED_Pin GPIO_PIN_2
#define LED_GPIO_Port GPIOB
#define DO1_PB12_Pin GPIO_PIN_12
#define DO1_PB12_GPIO_Port GPIOB
#define DO2_PB13_Pin GPIO_PIN_13
#define DO2_PB13_GPIO_Port GPIOB
#define DO3_PB14_Pin GPIO_PIN_14
#define DO3_PB14_GPIO_Port GPIOB
#define DO4_PB15_Pin GPIO_PIN_15
#define DO4_PB15_GPIO_Port GPIOB
#define NEXTION_TX_Pin GPIO_PIN_8
#define NEXTION_TX_GPIO_Port GPIOD
#define NEXTION_RX_Pin GPIO_PIN_9
#define NEXTION_RX_GPIO_Port GPIOD
#define USART6_TX_DATA_Pin GPIO_PIN_6
#define USART6_TX_DATA_GPIO_Port GPIOC
#define USART6_RX_DATA_Pin GPIO_PIN_7
#define USART6_RX_DATA_GPIO_Port GPIOC
#define ACHIQUE_CTRL_Pin GPIO_PIN_5
#define ACHIQUE_CTRL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* LED de prueba de enlace con la estacion de tierra. */
#define LED_RX_TIERRA_Pin GPIO_PIN_2
#define LED_RX_TIERRA_GPIO_Port GPIOB

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
