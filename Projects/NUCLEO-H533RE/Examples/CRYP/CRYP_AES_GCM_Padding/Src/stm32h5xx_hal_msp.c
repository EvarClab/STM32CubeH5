/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    CRYP/CRYP_AES_GCM_Padding/Src/stm32h5xx_hal_msp.c
  * @author  MCD Application Team
  * @brief   HAL MSP module.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{

  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  /* System interrupt init*/

  /* Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral */
  HAL_PWREx_DisableUCPDDeadBattery();

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

/**
  * @brief CRYP MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hcryp: CRYP handle pointer
  * @retval None
  */
void HAL_CRYP_MspInit(CRYP_HandleTypeDef* hcryp)
{
  if(hcryp->Instance==AES)
  {
    /* USER CODE BEGIN AES_MspInit 0 */

    /* USER CODE END AES_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_AES_CLK_ENABLE();
    /* USER CODE BEGIN AES_MspInit 1 */

    /* USER CODE END AES_MspInit 1 */

  }

}

/**
  * @brief CRYP MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hcryp: CRYP handle pointer
  * @retval None
  */
void HAL_CRYP_MspDeInit(CRYP_HandleTypeDef* hcryp)
{
  if(hcryp->Instance==AES)
  {
    /* USER CODE BEGIN AES_MspDeInit 0 */

    /* USER CODE END AES_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_AES_CLK_DISABLE();
    /* USER CODE BEGIN AES_MspDeInit 1 */

    /* USER CODE END AES_MspDeInit 1 */
  }

}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
