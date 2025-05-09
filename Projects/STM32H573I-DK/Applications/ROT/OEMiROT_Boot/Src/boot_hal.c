/**
  ******************************************************************************
  * @file    boot_hal.c
  * @author  MCD Application Team
  * @brief   This file contains  mcuboot stm32h5xx hardware specific implementation
  *
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "boot_hal_cfg.h"
#include "boot_hal.h"
#include "boot_hal_hash_ref.h"
#include "boot_hal_imagevalid.h"
#include "boot_hal_flowcontrol.h"
#include "mcuboot_config/mcuboot_config.h"
#include "uart_stdout.h"
#include "low_level_security.h"
#include "bootutil/boot_record.h"
#include "target_cfg.h"
#include "cmsis.h"
#include "Driver_Flash.h"
#include "region_defs.h"
#include "low_level_rng.h"
#include "low_level_obkeys.h"
#include "bootutil_priv.h"
#ifdef MCUBOOT_EXT_LOADER
#include "bootutil/crypto/sha256.h"
#define BUTTON_PORT                       GPIOC
#define BUTTON_CLK_ENABLE                 __HAL_RCC_GPIOC_CLK_ENABLE()
#define BUTTON_PIN                        GPIO_PIN_13
#endif /* MCUBOOT_EXT_LOADER */
extern ARM_DRIVER_FLASH FLASH_DEV_NAME;

typedef struct
{
  uint8_t Reserved1[248];
  uint8_t Magic[8];
} STM32xx_Descriptor_t;

STM32xx_Descriptor_t *DescriptorBase = (STM32xx_Descriptor_t *)STM32_DESCRIPTOR_BASE_NS_3;
RSSLIB_pFunc_TypeDef *Rss_lib_p = (RSSLIB_pFunc_TypeDef *)RSSLIB_PFUNC_3;

#if defined(MCUBOOT_DOUBLE_SIGN_VERIF)
/* Global variables to memorize images validation status */
#if (MCUBOOT_IMAGE_NUMBER == 1)
uint8_t ImageValidStatus[MCUBOOT_IMAGE_NUMBER] = {IMAGE_INVALID};
#elif (MCUBOOT_IMAGE_NUMBER == 2)
uint8_t ImageValidStatus[MCUBOOT_IMAGE_NUMBER] = {IMAGE_INVALID, IMAGE_INVALID};
#elif (MCUBOOT_IMAGE_NUMBER == 3)
uint8_t ImageValidStatus[MCUBOOT_IMAGE_NUMBER] = {IMAGE_INVALID, IMAGE_INVALID, IMAGE_INVALID};
#elif (MCUBOOT_IMAGE_NUMBER == 4)
uint8_t ImageValidStatus[MCUBOOT_IMAGE_NUMBER] = {IMAGE_INVALID, IMAGE_INVALID, IMAGE_INVALID, IMAGE_INVALID};
#else
#error "MCUBOOT_IMAGE_NUMBER not supported"
#endif /* (MCUBOOT_IMAGE_NUMBER == 1)  */
uint8_t ImageValidIndex = 0;
#endif /* MCUBOOT_DOUBLE_SIGN_VERIF */
#if defined(MCUBOOT_DOUBLE_SIGN_VERIF) || defined(MCUBOOT_USE_HASH_REF)
uint8_t ImageValidEnable = 0;
#endif /* MCUBOOT_DOUBLE_SIGN_VERIF || MCUBOOT_USE_HASH_REF */

#if defined(MCUBOOT_USE_HASH_REF)
#define BL2_HASH_REF_ADDR     (FLASH_HASH_REF_AREA_OFFSET)
uint8_t ImageValidHashUpdate = 0;
uint8_t ImageValidHashRef[MCUBOOT_IMAGE_NUMBER * SHA256_LEN] = {0};
#endif /* MCUBOOT_USE_HASH_REF */

#if defined(FLOW_CONTROL)
/* Global variable for Flow Control state */
volatile uint32_t uFlowProtectValue = FLOW_CTRL_INIT_VALUE;
#endif /* FLOW_CONTROL */
volatile uint32_t uFlowStage = FLOW_STAGE_CFG;

/*
#define ICACHE_MONITOR
*/
#if defined(ICACHE_MONITOR)
#define ICACHE_MONITOR_PRINT() printf("icache monitor - Hit: %x, Miss: %x\r\n", \
                                      HAL_ICACHE_Monitor_GetHitValue(), HAL_ICACHE_Monitor_GetMissValue());
#else
#define ICACHE_MONITOR_PRINT()
#endif /* ICACHE_MONITOR */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup BOOT_HAL_Private_Functions  Private Functions
  * @{
  */
static void boot_jump_to_RSS(uint32_t boot_jump_addr1, uint32_t boot_jump_addr2,
                             uint32_t vector_table, uint32_t MPU_region);
void icache_init(void);
void getDescriptorAdd(void);
/**
  * @}
  */
#if defined(MCUBOOT_EXT_LOADER)
/**
  * @brief This function manage the jump to boot loader application.
  * @note
  * @retval void
  */
void boot_platform_noimage(void)
{
  uint32_t rsslib_sec_jump_HDP_lvl3ns;

  BOOT_LOG_INF("Jumping to bootloader");
  BOOT_LOG_INF("Disconnect COM port if used by bootloader");

  /* Init RSS jump function descriptor */
  rsslib_sec_jump_HDP_lvl3ns = (uint32_t)(Rss_lib_p->S.JumpHDPLvl3NS);

  /* Check Flow control */
  FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_STAGE_2);
  uFlowStage = FLOW_STAGE_CFG;

  /* Update run time protections for application execution */
  LL_SECU_UpdateLoaderRunTimeProtections();


  /* Check Flow control */
  FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_STAGE_3_L);
  uFlowStage = FLOW_STAGE_CHK;

  /* Second function call to resist to basic hardware attacks */
  LL_SECU_UpdateLoaderRunTimeProtections();


  /* Check Flow control */
  FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_STAGE_4_L);

  /* Jump into BL through RSS */
  /* last parameter (0U) not used in RSSLIB_Sec_JumpHDPL3NS(BOOTLOADER_BASE_NS); */
  boot_jump_to_RSS((uint32_t)&boot_jump_to_RSS, rsslib_sec_jump_HDP_lvl3ns, (uint32_t) BOOTLOADER_BASE_NS, 0U);

  /* Avoid compiler to pop registers after having changed MSP */
#if !defined(__ICCARM__)
  __builtin_unreachable();
#else
    while (1);
#endif /* defined(__ICCARM__) */
}
#endif /* MCUBOOT_EXT_LOADER */
/**
  * @brief  Jumping to next image using RSS service, after erasing SRAM twice
  * @param  boot_jump_addr1 boot_jump_to_RSS address in order to execute twice SRAM erasing
  * @param  boot_jump_addr2 RSS service address
  * @param  vector table vector table of the next image
  * @param  MPU_region MPU region to be enabled (not applicable if RSS service is jumping to NS image)
  */
__attribute__((naked)) void boot_jump_to_RSS(uint32_t boot_jump_addr1, uint32_t boot_jump_addr2,
                                             uint32_t vector_table, uint32_t MPU_region)
{
  __ASM volatile(
#if !defined(__ICCARM__)
    ".syntax unified                 \n"
#endif
    "mov     r7, r0                  \n"
    "mov     r8, r1                  \n"
    "mov     r9, r2                  \n"
    "mov     r10, r3                 \n"
    "bl      boot_clear_bl2_ram_area \n" /* Clear RAM before jump */
    "movs    r0, #0                  \n" /* Clear registers: R0-R12, */
    "mov     r1, r0                  \n" /* except R7/R8/R9/R10 */
    "mov     r2, r0                  \n"
    "mov     r3, r0                  \n"
    "mov     r4, r0                  \n"
    "mov     r5, r0                  \n"
    "mov     r6, r0                  \n"
    "mov     r11, r0                 \n"
    "mov     r12, r0                 \n"
    "mov     lr,  r0                 \n"
    "mov     r0, r8                  \n"
    "mov     r1, r9                  \n"
    "mov     r2, r10                 \n"
    "mov     r8, r3                  \n"
    "mov     r9, r3                  \n"
    "mov     r10, r3                 \n"
    "bx      r7                      \n" /* Jump to RSS service */
  );
}
/**
  * @brief This function manage the jump to secure application.
  * @note
  * @retval void
  */
void boot_platform_quit(struct boot_arm_vector_table *vector)
{
    static struct boot_arm_vector_table *vt;
    uint32_t rsslib_sec_jump_HDP_lvl3;
#if defined(MCUBOOT_DOUBLE_SIGN_VERIF)
    uint32_t image_index;

   (void)fih_delay();
    /* Check again if images have been validated, to resist to basic hw attacks */
    for (image_index = 0; image_index < MCUBOOT_IMAGE_NUMBER; image_index++)
    {
        if (ImageValidStatus[image_index] != IMAGE_VALID)
        {
            BOOT_LOG_ERR("Error while double controlling images validation");
            Error_Handler();
        }
    }
#endif /* MCUBOOT_DOUBLE_SIGN_VERIF */

    /* Init RSS jump function descriptor */
    rsslib_sec_jump_HDP_lvl3 = (uint32_t)(Rss_lib_p->S.JumpHDPLvl3);

#if defined(MCUBOOT_USE_HASH_REF)
    /* Store new hash references in flash for next boot */
    if (ImageValidHashUpdate)
    {
        if (boot_hash_ref_store())
        {
            BOOT_LOG_ERR("Error while storing hash references");
            Error_Handler();
        }
    }
#endif /* MCUBOOT_USE_HASH_REF */

    /* Check Flow control state */
    FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_STAGE_2);
    uFlowStage = FLOW_STAGE_CFG;


    RNG_DeInit();

    ICACHE_MONITOR_PRINT()

#ifdef OEMIROT_ICACHE_ENABLE
    /* Invalidate ICache before jumping to application */
    if (HAL_ICACHE_Invalidate() != HAL_OK)
    {
        Error_Handler();
    }
#endif /* ICACHE_ENABLED */
    /* Update run time protections for application execution */
    LL_SECU_UpdateRunTimeProtections();

    vt = (struct boot_arm_vector_table *)vector;
    /* Check Flow control state */
    FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_STAGE_3_A);
    uFlowStage = FLOW_STAGE_CHK;

    /* Double the update of run time protections, to resist to basic hardware attacks */
    LL_SECU_UpdateRunTimeProtections();

    /* Check Flow control for dynamic protections */
    FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_STAGE_4_A);
    /*  change stack limit  */
    __set_MSPLIM(0);

    boot_jump_to_RSS((uint32_t)&boot_jump_to_RSS, rsslib_sec_jump_HDP_lvl3, (uint32_t) vt, 1U);
    /* Avoid compiler to pop registers after having changed MSP */
#if !defined(__ICCARM__)
    __builtin_unreachable();
#else
    while (1);
#endif /* defined(__ICCARM__) */
}


/**
  * @brief This function is called to clear all RAM area before jumping in
  * in Secure application .
  * @note
  * @retval void
  */
void boot_clear_bl2_ram_area(void)
{
    __IO uint32_t *pt = (uint32_t *)BL2_DATA_START;
    uint32_t index;

    for (index = 0; index < (BL2_DATA_SIZE / 4); index++)
    {
        pt[index] = 0;
    }

}

#if defined(MCUBOOT_USE_HASH_REF)
/**
  * @brief This function store all hash references in flash
  * @return 0 on success; nonzero on failure.
  */
int boot_hash_ref_store(void)
{
#if  defined(OEMUROT_ENABLE)
  OBK_Hdpl2Data OBK_hdpl2_data = { 0U };
#else
  OBK_Hdpl1Data OBK_hdpl1_data = { 0U };
#endif
  /* Read all OBK hdpl1/2 data & control SHA256 */
#if  defined(OEMUROT_ENABLE)
  if (OBK_ReadHdpl2Data(&OBK_hdpl2_data) !=  HAL_OK)
#else
  if (OBK_ReadHdpl1Data(&OBK_hdpl1_data) !=  HAL_OK)
#endif
  {
    return BOOT_EFLASH;
  }

  /* update hash references */
#if  defined(OEMUROT_ENABLE)
  memcpy(&OBK_hdpl2_data.Image0SHA256[0], ImageValidHashRef, (SHA256_LENGTH * MCUBOOT_IMAGE_NUMBER));
#else
  memcpy(&OBK_hdpl1_data.Image0SHA256[0], ImageValidHashRef, (SHA256_LENGTH * MCUBOOT_IMAGE_NUMBER));
#endif

  /* Update all OBK hdpl1 data with associated hash references */
#if  defined(OEMUROT_ENABLE)
  if (OBK_UpdateHdpl2Data(&OBK_hdpl2_data) != HAL_OK)
#else
  if (OBK_UpdateHdpl1Data(&OBK_hdpl1_data) != HAL_OK)
#endif
  {
    return BOOT_EFLASH;
  }

  return 0;
}

/**
  * @brief This function load all hash references from flash
  * @return 0 on success; nonzero on failure.
  */
int boot_hash_ref_load(void)
{
#if  defined(OEMUROT_ENABLE)
  OBK_Hdpl2Data OBK_hdpl2_data = { 0U };
#else
  OBK_Hdpl1Data OBK_hdpl1_data = { 0U };
#endif

  /* Read all OBK hdpl1 data & control SHA256 */
#if  defined(OEMUROT_ENABLE)
  if (OBK_ReadHdpl2Data(&OBK_hdpl2_data) != HAL_OK)
#else
  if (OBK_ReadHdpl1Data(&OBK_hdpl1_data) != HAL_OK)
#endif
  {
    return BOOT_EFLASH;
  }

  /* update hash references */
#if  defined(OEMUROT_ENABLE)
  memcpy(ImageValidHashRef, &OBK_hdpl2_data.Image0SHA256[0], (SHA256_LENGTH * MCUBOOT_IMAGE_NUMBER));
#else
  memcpy(ImageValidHashRef, &OBK_hdpl1_data.Image0SHA256[0], (SHA256_LENGTH * MCUBOOT_IMAGE_NUMBER));
#endif

  return 0;
}

/**
  * @brief This function set one hash reference in ram
  * @param hash_ref hash reference to update
  * @param size size of the hash references
  * @param image_index index of image corresponding to hash reference
  * @return 0 on success; nonzero on failure.
  */
int boot_hash_ref_set(uint8_t *hash_ref, uint8_t size, uint8_t image_index)
{
  /* Check size */
  if (size != SHA256_LEN)
  {
    return BOOT_EFLASH;
  }

  /* Check image index */
  if (image_index >= MCUBOOT_IMAGE_NUMBER)
  {
    return BOOT_EFLASH;
  }

  /* Set hash reference */
  memcpy(ImageValidHashRef + (image_index * SHA256_LEN), hash_ref, SHA256_LEN);

  /* Memorize that hash references will have to be updated in flash (later) */
  ImageValidHashUpdate++;

  return 0;
}

/**
  * @brief This function get one hash reference from ram
  * @param hash_ref hash reference to get
  * @param size size of the hash reference
  * @param image_index index of image corresponding to hash reference
  * @return 0 on success; nonzero on failure.
  */
int boot_hash_ref_get(uint8_t *hash_ref, uint8_t size, uint8_t image_index)
{
  /* Check size */
  if (size != SHA256_LEN)
  {
    return BOOT_EFLASH;
  }

  /* Check image index */
  if (image_index >= MCUBOOT_IMAGE_NUMBER)
  {
    return BOOT_EFLASH;
  }

  /* Get hash reference */
  memcpy(hash_ref, ImageValidHashRef + (image_index * SHA256_LEN), SHA256_LEN);

  return 0;
}
#endif /* MCUBOOT_USE_HASH_REF */

/**
  * @brief This function configures and enables the ICache.
  * @note
  * @retval execution_status
  */
void icache_init(void)
{
#ifdef ICACHE_MONITOR
    if (HAL_ICACHE_Monitor_Reset(ICACHE_MONITOR_HIT_MISS) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_ICACHE_Monitor_Start(ICACHE_MONITOR_HIT_MISS) != HAL_OK)
    {
        Error_Handler();
    }
#endif /* ICACHE_MONITOR */
    ICACHE_MONITOR_PRINT()

    /* Enable ICache */
    if (HAL_ICACHE_Enable() != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief This function get the descriptor base address shall be used.
  * @note
  * @retval void
  */
void getDescriptorAdd(void)
{
    char ReadMagic[9] = {0};
    for(int i=0; i<8; i++)
    {
      ReadMagic[i] = DescriptorBase->Magic[i];
    }
    if(strcmp(ReadMagic, "DSCTR484") != 0)
    {
      DescriptorBase = (STM32xx_Descriptor_t *)STM32_DESCRIPTOR_BASE_NS_2;
      Rss_lib_p = (RSSLIB_pFunc_TypeDef *)RSSLIB_PFUNC_2;
      for(int i=0; i<8; i++)
      {
        ReadMagic[i] = DescriptorBase->Magic[i];
      }
      if(strcmp(ReadMagic, "DSCTR484") != 0)
      {
        DescriptorBase = (STM32xx_Descriptor_t *)STM32_DESCRIPTOR_BASE_NS_1;
        Rss_lib_p = (RSSLIB_pFunc_TypeDef *)RSSLIB_PFUNC_1;
      }
    }
}

/* exported variables --------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/**
  * @brief  Platform init
  * @param  None
  * @retval status
  */
int32_t boot_platform_init(void)
{
#if defined(MCUBOOT_EXT_LOADER)
    GPIO_InitTypeDef GPIO_Init;
#endif /* MCUBOOT_EXT_LOADER */
    /* STM32H5xx HAL library initialization:
         - Systick timer is configured by default as source of time base, but user
               can eventually implement his proper time base source (a general purpose
               timer for example or other time source), keeping in mind that Time base
               duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
               handled in milliseconds basis.
         - Set NVIC Group Priority to 3
         - Low Level Initialization
       */
    HAL_Init();
#ifdef MCUBOOT_HAVE_LOGGING
    /* Init for log */
    stdio_init();
#endif /*  MCUBOOT_HAVE_LOGGING */

#ifdef OEMIROT_ICACHE_ENABLE
    /* Configure and enable ICache */
    icache_init();
#endif /* ICACHE_ENABLED */

    /* Start HW randomization */
    RNG_Init();

    (void)fih_delay_init();
#if  defined(OEMUROT_ENABLE)
    /* disable and clean STiROT mpu config */
    LL_SECU_DisableCleanMpu();
#endif

    /* Apply Run time Protection */
    LL_SECU_ApplyRunTimeProtections();
    /* Check static protections */
    LL_SECU_CheckStaticProtections();

    /* get descriptor address to support new and old descriptor */
    getDescriptorAdd();

    /* Init DHUK */
    OBK_InitDHUK();

    /* Configure keys */
#if  defined(OEMUROT_ENABLE)
    OBK_ReadHdpl2Config(&OBK_Hdpl2_Cfg);
#else
    OBK_ReadHdpl1Config(&OBK_Hdpl1_Cfg);
#endif

    /* Check Flow control state */
    FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_STAGE_1);
    uFlowStage = FLOW_STAGE_CHK;
    /* Double protections apply / check to resist to basic fault injections */
    /* Apply Run time Protection */
   (void)fih_delay();
    LL_SECU_ApplyRunTimeProtections();
    /* Check static protections */
    LL_SECU_CheckStaticProtections();

    /* Control Hdpl1/2 config */
#if  defined(OEMUROT_ENABLE)
    OBK_VerifyHdpl2Config(&OBK_Hdpl2_Cfg);
#else
    OBK_VerifyHdpl1Config(&OBK_Hdpl1_Cfg);
#endif

    if (FLASH_DEV_NAME.Initialize(NULL) != ARM_DRIVER_OK)
    {
        BOOT_LOG_ERR("Error while initializing Flash Interface");
        Error_Handler();
    }

#if defined(MCUBOOT_USE_HASH_REF)
    /* Load all images hash references (for mcuboot) */
    if (boot_hash_ref_load())
    {
        BOOT_LOG_ERR("Error while loading Hash references from FLash");
        Error_Handler();
    }
#endif

#if defined(MCUBOOT_EXT_LOADER)
    /* configure Button pin */
    BUTTON_CLK_ENABLE;
    GPIO_Init.Pin       = BUTTON_PIN;
    GPIO_Init.Mode      = 0;
    GPIO_Init.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_Init.Pull      = GPIO_NOPULL;
    GPIO_Init.Alternate = 0;
    HAL_GPIO_Init(BUTTON_PORT, &GPIO_Init);
    /* read pin value */
    if (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_SET)
    {
        boot_platform_noimage();
    }
#endif /* MCUBOOT_EXT_LOADER */
    return 0;
}

/**
  * @brief  This function is executed in case of error occurrence.
  *         This function does not return.
  * @param  None
  * @retval None
  */
#ifdef OEMIROT_ERROR_HANDLER_STOP_EXEC
/* Place code in a specific section */
#if defined(__ICCARM__)
#pragma default_function_attributes = @ ".BL2_Error_Code"
#else
__attribute__((section(".BL2_Error_Code")))
#endif /* __ICCARM__ */
void Error_Handler(void)
{
    while(1);
}
#else /* OEMIROT_ERROR_HANDLER_STOP_EXEC */
/* Place code in a specific section */
#if defined(__ICCARM__)
#pragma default_function_attributes = @ ".BL2_Error_Code"
__NO_RETURN void Error_Handler(void)
#else
__attribute__((section(".BL2_Error_Code")))
void Error_Handler(void)
#endif
{
    /* It is customizeable */
    NVIC_SystemReset();
#if !defined(__ICCARM__)
    /* Avoid bx lr instruction (for fault injection) */
    __builtin_unreachable();
#endif /* defined(__ICCARM__) */
}
#endif /* OEMIROT_ERROR_HANDLER_STOP_EXEC */

/* Stop placing data in specified section */
#if defined(__ICCARM__)
#pragma default_function_attributes =
#endif /* __ICCARM__ */

#if defined(__ARMCC_VERSION)
/* reimplement the function to reach Error Handler */
void __aeabi_assert(const char *expr, const char *file, int line)
{
#ifdef OEMIROT_DEV_MODE
    BOOT_LOG_INF("assertion \" %s \" failed: file %s %d\n", expr, file, line);
#endif /*  OEMIROT_DEV_MODE  */
    Error_Handler();
}
#endif  /*  __ARMCC_VERSION */
#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    Error_Handler();
}
#endif /* USE_FULL_ASSERT */
