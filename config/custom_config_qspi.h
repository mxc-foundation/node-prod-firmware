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
 * Copyright (C) 2015-2017 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef CUSTOM_CONFIG_QSPI_H_
#define CUSTOM_CONFIG_QSPI_H_

#include "bsp_definitions.h"

#define CONFIG_USE_BLE
#undef CONFIG_USE_FTDF


#define dg_configUSE_LP_CLK                     LP_CLK_32768
#define dg_configEXEC_MODE                      MODE_IS_CACHED
#define dg_configCODE_LOCATION                  NON_VOLATILE_IS_FLASH
#define dg_configEXT_CRYSTAL_FREQ               EXT_CRYSTAL_IS_16M

#define dg_configIMAGE_SETUP                    DEVELOPMENT_MODE
#define dg_configEMULATE_OTP_COPY               (0)

#define dg_configUSER_CAN_USE_TIMER1            (0)

#define dg_configOPTIMAL_RETRAM                 (1)

#if (dg_configOPTIMAL_RETRAM == 1)
        #if (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A)
                #define dg_configMEM_RETENTION_MODE             (0x1B)
                #define dg_configSHUFFLING_MODE                 (0x0)
        #else
                #define dg_configMEM_RETENTION_MODE             (0x07)
                #define dg_configSHUFFLING_MODE                 (0x0)
        #endif
#endif

#define dg_configUSE_WDOG                       (1)


#define dg_configFLASH_CONNECTED_TO             (FLASH_CONNECTED_TO_1V8)
#define dg_configFLASH_POWER_DOWN               (0)

#define dg_configPOWER_1V8_ACTIVE               (1)
#define dg_configPOWER_1V8_SLEEP                (1)

#define dg_configBATTERY_TYPE                   (BATTERY_TYPE_CUSTOM)
#define dg_configBATTERY_CHARGE_VOLTAGE         0xA     // 4.2V
#define dg_configBATTERY_TYPE_CUSTOM_ADC_VOLTAGE        (3439)
#define dg_configPRECHARGING_THRESHOLD          (2462)  // 3.006V
#define dg_configCHARGING_THRESHOLD             (2498)  // 3.05V
#define dg_configBATTERY_CHARGE_CURRENT         4       // 60mA
#define dg_configBATTERY_PRECHARGE_CURRENT      20      // 2.1mA
#define dg_configBATTERY_CHARGE_NTC             1       // disabled
#define dg_configPRECHARGING_TIMEOUT            (30 * 60 * 100)  // N x 10msec

#define dg_configUSE_SOC                        1
/* Uncomment the following line to enable State-of-Charge debugging or performance test */
//#define DEBUG_SOC
#if defined(DEBUG_SOC)
        #define CONFIG_RETARGET
#endif

#define dg_configUSE_USB                        1
#define dg_configUSE_USB_CHARGER                1
#define dg_configALLOW_CHARGING_NOT_ENUM        1
#define dg_configUSE_NOT_ENUM_CHARGING_TIMEOUT  0

#define dg_configUSE_ProDK                      (1)

#define dg_configUSE_SW_CURSOR                  (1)

#define dg_configCACHEABLE_QSPI_AREA_LEN        (NVMS_PARAM_PART_start - MEMORY_QSPIF_BASE)

/*************************************************************************************************\
 * Memory specific config
 */
#define dg_configQSPI_CACHED_OPTIMAL_RETRAM_0_SIZE_AE   ( 64 * 1024)
#define dg_configQSPI_CACHED_RAM_SIZE_AE                ( 32 * 1024)
#define dg_configQSPI_CACHED_RETRAM_0_SIZE_AE           ( 96 * 1024)


/*************************************************************************************************\
 * FreeRTOS specific config
 */
#define OS_FREERTOS                              /* Define this to use FreeRTOS */
#if (dg_configUSE_SOC)
        #if defined(DEBUG_SOC)
                #define configTOTAL_HEAP_SIZE                    12300   /* This is the FreeRTOS Total Heap Size */
        #else
                #define configTOTAL_HEAP_SIZE                    11200   /* This is the FreeRTOS Total Heap Size */
        #endif
#else
        #define configTOTAL_HEAP_SIZE                    11000   /* This is the FreeRTOS Total Heap Size */
#endif

/*************************************************************************************************\
 * Peripheral specific config
 */
#define dg_configFLASH_ADAPTER                  1
#define dg_configNVMS_ADAPTER                   1
#define dg_configNVMS_VES                       1
#define dg_configNVPARAM_ADAPTER                1
#define dg_configGPADC_ADAPTER                  1

#define defaultBLE_ATT_DB_CONFIGURATION         (0x10)  // with "Peripheral Preferred Connection Parameters"
#define defaultBLE_PPCP_INTERVAL_MIN            (BLE_CONN_INTERVAL_FROM_MS(500))    // 500 ms
#define defaultBLE_PPCP_INTERVAL_MAX            (BLE_CONN_INTERVAL_FROM_MS(750))    // 750 ms
#define defaultBLE_PPCP_SLAVE_LATENCY           (0)                                 // 0 events
#define defaultBLE_PPCP_SUP_TIMEOUT             (BLE_SUPERVISION_TMO_FROM_MS(6000)) // 6000 ms

/*************************************************************************************************\
 * BLE device config
 */
#define dg_configBLE_CENTRAL                    (0)
#define dg_configBLE_GATT_CLIENT                (0)
#define dg_configBLE_OBSERVER                   (0)
#define dg_configBLE_BROADCASTER                (0)
#define dg_configBLE_L2CAP_COC                  (0)




/* Include bsp default values */
#include "bsp_defaults.h"
/* Include memory layout */
#include "bsp_memory_layout.h"
#endif /* CUSTOM_CONFIG_QSPI_H_ */

/**
\}
\}
\}
*/
