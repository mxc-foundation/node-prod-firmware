/**
\addtogroup BSP
\{
\addtogroup CONFIG
\{
\addtogroup CUSTOM
\{
*/

/**
****************************************************************************************
*
* @file custom_config_qspi.h
*
* @brief Board Support Package. User Configuration file for cached QSPI mode.
*
* Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
* program includes Confidential, Proprietary Information and is a Trade Secret of
* Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
* unless authorized in writing. All Rights Reserved.
*
* <black.orca.support@diasemi.com> and contributors.
*
****************************************************************************************
*/

#ifndef CUSTOM_CONFIG_QSPI_H_
#define CUSTOM_CONFIG_QSPI_H_

#include "bsp_definitions.h"

#define __HEAP_SIZE  0x0100
#define __STACK_SIZE 0x0100

#define dg_configTEMPSENS_ADAPTER               1
#define dg_configGPADC_ADAPTER                  1

#define SERIAL_CONSOLE_RETARGET                 1

#define dg_configPOWER_CONFIG                   (POWER_CONFIGURATION_2)

#define dg_configUSE_LP_CLK                     LP_CLK_32768
#define dg_configEXEC_MODE                      MODE_IS_CACHED
#define dg_configCODE_LOCATION                  NON_VOLATILE_IS_FLASH
#define dg_configEXT_CRYSTAL_FREQ               EXT_CRYSTAL_IS_16M

#define dg_configIMAGE_SETUP                    DEVELOPMENT_MODE
#define dg_configEMULATE_OTP_COPY               (0)

#define dg_configUSER_CAN_USE_TIMER1            (0)

#define dg_configMEM_RETENTION_MODE             (0x1F)
#define dg_configMEM_RETENTION_MODE_PRESERVE_IMAGE       (0x1F)
#define dg_configSHUFFLING_MODE                 (0x3)

#define dg_configUSE_WDOG                       (0)

#define dg_configUSE_DCDC                       (1)

#define dg_configPOWER_FLASH                    (1)
#define dg_configFLASH_POWER_DOWN               (0)
#define dg_configFLASH_POWER_OFF                (0)

#define dg_configBATTERY_TYPE                   (BATTERY_TYPE_LIMN2O4)
#define dg_configBATTERY_CHARGE_CURRENT         2       // 30mA
#define dg_configBATTERY_PRECHARGE_CURRENT      20      // 2.1mA
#define dg_configBATTERY_CHARGE_NTC             1       // disabled

#define dg_configUSE_USB_CHARGER                0
#define dg_configALLOW_CHARGING_NOT_ENUM        1

#define dg_configUSE_ProDK                      (1)

#define dg_configUSE_SW_CURSOR                  (1)

#define dg_configCACHEABLE_QSPI_AREA_LEN        NVMS_PARAM_PART_start


/*************************************************************************************************\
 * FreeRTOS specific config
 */
#define OS_FREERTOS                              /* Define this to use FreeRTOS */
#define configTOTAL_HEAP_SIZE                    14000   /* This is the FreeRTOS Total Heap Size */

/*************************************************************************************************\
 * Peripheral specific config
 */
#define dg_configUSE_HW_I2C                     (1)
#define dg_configUSE_HW_IRGEN                   (1)
#define dg_configUSE_HW_QUAD                    (1)
#define dg_configUSE_HW_RF                      (0)
#define dg_configUSE_HW_SPI                     (1)
#define dg_configUSE_HW_TIMER0                  (1)
#define dg_configUSE_HW_TIMER2                  (1)

#define dg_configI2C_ADAPTER                    (1)
#define dg_configSPI_ADAPTER                    (1)
#define dg_configUART_ADAPTER                   (1)
#define dg_configRF_ADAPTER                     (0)

/* Include bsp default values */
#include "bsp_defaults.h"

#endif /* CUSTOM_CONFIG_QSPI_H_ */

/**
\}
\}
\}
*/
