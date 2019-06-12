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
 * @file custom_config_qspi_suota.h
 *
 * @brief Board Support Package. User Configuration file for cached QSPI mode.
 *
 * Copyright (C) 2015-2017 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef CUSTOM_CONFIG_QSPI_SUOTA_H_
#define CUSTOM_CONFIG_QSPI_SUOTA_H_

#include "bsp_definitions.h"

#define CONFIG_USE_BLE
#undef CONFIG_USE_FTDF


#define dg_configUSE_LP_CLK                     LP_CLK_32768
#define dg_configEXEC_MODE                      MODE_IS_CACHED
#define dg_configCODE_LOCATION                  NON_VOLATILE_IS_FLASH
#define dg_configEXT_CRYSTAL_FREQ               EXT_CRYSTAL_IS_16M

#define dg_configIMAGE_SETUP                    PRODUCTION_MODE
//#define dg_configIMAGE_SETUP                    DEVELOPMENT_MODE

#if dg_configIMAGE_SETUP == PRODUCTION_MODE
#define dg_configSKIP_MAGIC_CHECK_AT_START      (1)
//#define dg_configENABLE_DEBUGGER                (0)
#endif

#define dg_configEMULATE_OTP_COPY               (0)

#define dg_configIMAGE_FLASH_OFFSET             (0x20000)
#define dg_configSUOTA_SUPPORT                  (1)

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
//#define dg_configBATTERY_LOW_LEVEL              (2457)  // 3V
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
 * Peripheral specific config
 */
#define dg_configUSE_HW_I2C                     1
#define dg_configUSE_HW_IRGEN                   1
#define dg_configUSE_HW_QUAD                    1
#define dg_configUSE_HW_RF                      1
#define dg_configUSE_HW_SPI                     1
#define dg_configUSE_HW_TRNG                    1

#define dg_configUSE_HW_TIMER1                  1
#define dg_configUSE_HW_TIMER2                  1

#define dg_configFLASH_ADAPTER                  1
#define dg_configNVMS_ADAPTER                   1
#define dg_configNVMS_VES                       1
#define dg_configNVPARAM_ADAPTER                1
#define dg_configGPADC_ADAPTER                  1

#define dg_configI2C_ADAPTER                    1
#define dg_configSPI_ADAPTER                    1
#define dg_configUART_ADAPTER                   1
#define dg_configRF_ADAPTER                     1
#define dg_configTEMPSENS_ADAPTER               1

#define HW_UART_ENABLE_USER_ISR

#define CONFIG_CUSTOM_PRINT
//#define CONFIG_NO_PRINT
//#define CONFIG_RTT

#ifdef CONFIG_CUSTOM_PRINT
#define __HEAP_SIZE                             0x600
#endif

/* LMIC */
#define CFG_sx1276_radio

#define defaultBLE_ATT_DB_CONFIGURATION         (0x10)  // with "Peripheral Preferred Connection Parameters"
#define defaultBLE_PPCP_INTERVAL_MIN            (BLE_CONN_INTERVAL_FROM_MS(500))    // 500 ms
#define defaultBLE_PPCP_INTERVAL_MAX            (BLE_CONN_INTERVAL_FROM_MS(750))    // 750 ms
#define defaultBLE_PPCP_SLAVE_LATENCY           (0)                                 // 0 events
#define defaultBLE_PPCP_SUP_TIMEOUT             (BLE_SUPERVISION_TMO_FROM_MS(6000)) // 6000 ms

#define BLE_MAX_MISSES_ALLOWED                  (3)
#define BLE_MAX_DELAYS_ALLOWED                  (3)

/* Use dynamic list for BLE events instead of FreeRTOS queues */
#define BLE_MGR_USE_EVT_LIST                    (1)

/*
 * SUOTA loader configuration:
 * - To enable SUOTA over GATT only, set SUOTA_VERSION to any version >= SUOTA_VERSION_1_1
 *      and leave SUOTA_PSM undefined.
 * - To enable SUOTA over GATT and L2CAP CoC, set SUOTA_VERSION to any version >= SUOTA_VERSION_1_2
 *      and also define SUOTA_PSM to match the desired PSM. In this case the central device
 *      can use either of both according to its preference.
 */
#define SUOTA_VERSION   SUOTA_VERSION_1_3
#define SUOTA_PSM       0x81

#define USE_PARTITION_TABLE_1MB_WITH_SUOTA

/*************************************************************************************************\
 * FreeRTOS specific config
 */
#define OS_FREERTOS                              /* Define this to use FreeRTOS */

#if SUOTA_PSM
        #define SUOTA_HEAP_OVERHEAD     (3200)
#else
        #define SUOTA_HEAP_OVERHEAD     (0)
#endif

#if (dg_configUSE_SOC)
        #if defined(DEBUG_SOC)
                #ifndef RELEASE_BUILD
                        /* DEBUG SOC should not be used in release builds */
                        #define configTOTAL_HEAP_SIZE   (12900 + SUOTA_HEAP_OVERHEAD) /* This is the FreeRTOS Total Heap Size */
                #else
                        #error "DEBUG SOC and SUOTA over L2CAP cannot be used in release target together"
                #endif
        #else
                #define configTOTAL_HEAP_SIZE   (11100 + SUOTA_HEAP_OVERHEAD)   /* This is the FreeRTOS Total Heap Size */
        #endif
#else
        #define configTOTAL_HEAP_SIZE           (11000 + SUOTA_HEAP_OVERHEAD)   /* This is the FreeRTOS Total Heap Size */
#endif

/*************************************************************************************************\
 * BLE device config
 */
#define dg_configBLE_CENTRAL                    (0)
#define dg_configBLE_GATT_CLIENT                (0)
#define dg_configBLE_OBSERVER                   (0)
#define dg_configBLE_BROADCASTER                (0)
#ifndef SUOTA_PSM
#define dg_configBLE_L2CAP_COC                  (0)
#endif


/* Include bsp default values */
#include "bsp_defaults.h"
/* Include memory layout */
#include "bsp_memory_layout.h"
#endif /* CUSTOM_CONFIG_QSPI_SUOTA_H_ */

/**
\}
\}
\}
*/
