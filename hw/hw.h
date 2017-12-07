#ifndef __HW_LORA_H__
#define __HW_LORA_H__

#ifndef HW_VERSION
#define HW_VERSION	HW_SOM_1_0
#endif

#define HW_MATCHSTICK_1_0a	0x010a
#define HW_MATCHSTICK_2_0a	0x020a
#define HW_SOM_1_0		0x1100

#define HW_IS_MATCHSTICK	((HW_VERSION & 0xf000) == 0x0000)
#define HW_IS_SOM		((HW_VERSION & 0xf000) == 0x1000)

#if HW_IS_SOM

#if HW_VERSION == HW_SOM_1_0

#define HW_VER_STRING		"DevKit 1.0"

#define FEATURE_LED_RGB
#define FEATURE_POWER_SUPPLY
#define FEATURE_SENSOR
#define FEATURE_SENSOR_TEMP
#define FEATURE_SENSOR_TEMP_PCT2075
#define FEATURE_SENSOR_LIGHT
#define FEATURE_USER_BUTTON

#define INITIAL_SLEEP_MODE	pm_mode_idle

#define HW_PS_EN_PORT		HW_GPIO_PORT_3
#define HW_PS_EN_PIN		HW_GPIO_PIN_4

#define HW_LORA_SPI_DI_PORT	HW_GPIO_PORT_3
#define HW_LORA_SPI_DI_PIN	HW_GPIO_PIN_7
#define HW_LORA_SPI_DO_PORT	HW_GPIO_PORT_3
#define HW_LORA_SPI_DO_PIN	HW_GPIO_PIN_6
#define HW_LORA_SPI_CS_PORT	HW_GPIO_PORT_3
#define HW_LORA_SPI_CS_PIN	HW_GPIO_PIN_5
#define HW_LORA_SPI_CLK_PORT	HW_GPIO_PORT_4
#define HW_LORA_SPI_CLK_PIN	HW_GPIO_PIN_0
#define HW_LORA_RX_PORT		HW_GPIO_PORT_4
#define HW_LORA_RX_PIN		HW_GPIO_PIN_7
#define HW_LORA_TX_PORT		HW_GPIO_PORT_3
#define HW_LORA_TX_PIN		HW_GPIO_PIN_0
#define HW_LORA_REST_PORT	HW_GPIO_PORT_1
#define HW_LORA_REST_PIN	HW_GPIO_PIN_0
#define HW_LORA_DIO0_PORT	HW_GPIO_PORT_1
#define HW_LORA_DIO0_PIN	HW_GPIO_PIN_5
#define HW_LORA_DIO1_PORT	HW_GPIO_PORT_1
#define HW_LORA_DIO1_PIN	HW_GPIO_PIN_2
#define HW_LORA_DIO2_PORT	HW_GPIO_PORT_1
#define HW_LORA_DIO2_PIN	HW_GPIO_PIN_7

#define HW_CONSOLE_UART_TX_PORT	HW_GPIO_PORT_1
#define HW_CONSOLE_UART_TX_PIN	HW_GPIO_PIN_4
#define HW_CONSOLE_UART_RX_PORT	HW_GPIO_PORT_1
#define HW_CONSOLE_UART_RX_PIN	HW_GPIO_PIN_6

#define HW_USER_BTN_PORT	HW_GPIO_PORT_3
#define HW_USER_BTN_PIN		HW_GPIO_PIN_2
#define HW_USER_BTN_ACTIVE	HW_WKUP_PIN_STATE_HIGH

#define HW_IOX_I2C_ADDR		0x20
#define HW_SENSOR_TEMP_I2C_ADDR	0x49
#define HW_SENSOR_GROVE_LIGHT_I2C_ADDR	0x29
#define HW_I2C_SCL_PORT		HW_GPIO_PORT_4
#define HW_I2C_SCL_PIN		HW_GPIO_PIN_3
#define HW_I2C_SDA_PORT		HW_GPIO_PORT_4
#define HW_I2C_SDA_PIN		HW_GPIO_PIN_2

#else /* !HW_SOM_1_0 */
#error Unsupported SOM version
#endif /* HW_SOM_1_0 */

#elif HW_IS_MATCHSTICK

#define FEATURE_BATTERY
#define FEATURE_LED_RG
#define FEATURE_POWER_SUPPLY
#define FEATURE_POWER_SUPPLY_MULTI
#define FEATURE_SENSOR
#define FEATURE_SENSOR_GPS
#define FEATURE_SENSOR_TEMP
#define FEATURE_SENSOR_TEMP_INTERNAL
#define FEATURE_USER_BUTTON

#define INITIAL_SLEEP_MODE	pm_mode_extended_sleep

#if HW_VERSION == HW_MATCHSTICK_1_0a

#define HW_VER_STRING		"MatchStick 1.0a"

#define HW_CONSOLE_UART_TX_PORT	HW_GPIO_PORT_4
#define HW_CONSOLE_UART_TX_PIN	HW_GPIO_PIN_5
#define HW_LORA_SPI_CLK_PORT	HW_GPIO_PORT_1
#define HW_LORA_SPI_CLK_PIN	HW_GPIO_PIN_0

#elif HW_VERSION == HW_MATCHSTICK_2_0a

#define HW_VER_STRING		"MatchStick 2.0a"

#define HW_CONSOLE_UART_TX_PORT	HW_GPIO_PORT_1
#define HW_CONSOLE_UART_TX_PIN	HW_GPIO_PIN_0
#define HW_LORA_SPI_CLK_PORT	HW_GPIO_PORT_1
#define HW_LORA_SPI_CLK_PIN	HW_GPIO_PIN_7

#define HW_SENSOR_MPU_I2C_ADDR	0x68
#define HW_SENSOR_MPU_INT_PORT	HW_GPIO_PORT_3
#define HW_SENSOR_MPU_INT_PIN	HW_GPIO_PIN_2
#define HW_I2C_SCL_PORT		HW_GPIO_PORT_1
#define HW_I2C_SCL_PIN		HW_GPIO_PIN_3
#define HW_I2C_SDA_PORT		HW_GPIO_PORT_0
#define HW_I2C_SDA_PIN		HW_GPIO_PIN_7

#else
#error Unsupported Matchstick version
#endif

#define HW_CONSOLE_UART_RX_PORT	HW_GPIO_PORT_4
#define HW_CONSOLE_UART_RX_PIN	HW_GPIO_PIN_4

#define HW_PS_EN_PORT		HW_GPIO_PORT_2
#define HW_PS_EN_PIN		HW_GPIO_PIN_3

#define HW_LORA_EN_PS_PORT	HW_GPIO_PORT_1
#define HW_LORA_EN_PS_PIN	HW_GPIO_PIN_6
#define HW_LORA_EN_LDO_PORT	HW_GPIO_PORT_3
#define HW_LORA_EN_LDO_PIN	HW_GPIO_PIN_5
#define HW_LORA_SPI_DI_PORT	HW_GPIO_PORT_1
#define HW_LORA_SPI_DI_PIN	HW_GPIO_PIN_2
#define HW_LORA_SPI_DO_PORT	HW_GPIO_PORT_1
#define HW_LORA_SPI_DO_PIN	HW_GPIO_PIN_5
#define HW_LORA_SPI_CS_PORT	HW_GPIO_PORT_4
#define HW_LORA_SPI_CS_PIN	HW_GPIO_PIN_1
#define HW_LORA_RX_PORT		HW_GPIO_PORT_4
#define HW_LORA_RX_PIN		HW_GPIO_PIN_3
#define HW_LORA_TX_PORT		HW_GPIO_PORT_4
#define HW_LORA_TX_PIN		HW_GPIO_PIN_2
#define HW_LORA_REST_PORT	HW_GPIO_PORT_1
#define HW_LORA_REST_PIN	HW_GPIO_PIN_4
#define HW_LORA_DIO0_PORT	HW_GPIO_PORT_4
#define HW_LORA_DIO0_PIN	HW_GPIO_PIN_0
#define HW_LORA_DIO1_PORT	HW_GPIO_PORT_3
#define HW_LORA_DIO1_PIN	HW_GPIO_PIN_7
#define HW_LORA_DIO2_PORT	HW_GPIO_PORT_3
#define HW_LORA_DIO2_PIN	HW_GPIO_PIN_6

#define HW_SENSOR_EN_PORT	HW_GPIO_PORT_4
#define HW_SENSOR_EN_PIN	HW_GPIO_PIN_6
#define HW_SENSOR_BACKUP_PORT	HW_GPIO_PORT_3
#define HW_SENSOR_BACKUP_PIN	HW_GPIO_PIN_4
#define HW_IMU_BACKUP_PORT	HW_GPIO_PORT_2
#define HW_IMU_BACKUP_PIN	HW_GPIO_PIN_4

#define HW_SENSOR_UART_RX_PORT	HW_GPIO_PORT_4
#define HW_SENSOR_UART_RX_PIN	HW_GPIO_PIN_7
#define HW_SENSOR_UART_TX_PORT	HW_GPIO_PORT_3
#define HW_SENSOR_UART_TX_PIN	HW_GPIO_PIN_0

#define HW_USER_BTN_PORT	HW_GPIO_PORT_3
#define HW_USER_BTN_PIN		HW_GPIO_PIN_3
#define HW_USER_BTN_ACTIVE	HW_WKUP_PIN_STATE_LOW

#else
#error Unsupported HW_VERSION
#endif

#define HW_LORA_SPI_NO		2

#define __DEFINE_HW_LORA_SPI_INT(x)	HW_SPI ## x
#define __DEFINE_HW_LORA_SPI(x)		__DEFINE_HW_LORA_SPI_INT(x)
#define HW_LORA_SPI			__DEFINE_HW_LORA_SPI(HW_LORA_SPI_NO)

#if HW_LORA_SPI_NO == 1
#define HW_LORA_GPIO_FUNC_SPI_CLK	HW_GPIO_FUNC_SPI_CLK
#define HW_LORA_GPIO_FUNC_SPI_DI	HW_GPIO_FUNC_SPI_DI
#define HW_LORA_GPIO_FUNC_SPI_DO	HW_GPIO_FUNC_SPI_DO
#elif HW_LORA_SPI_NO == 2
#define HW_LORA_GPIO_FUNC_SPI_CLK	HW_GPIO_FUNC_SPI2_CLK
#define HW_LORA_GPIO_FUNC_SPI_DI	HW_GPIO_FUNC_SPI2_DI
#define HW_LORA_GPIO_FUNC_SPI_DO	HW_GPIO_FUNC_SPI2_DO
#else
#error "Invalid HW_LORA_SPI_NO definition"
#endif

#endif /* __HW_LORA_H__ */
