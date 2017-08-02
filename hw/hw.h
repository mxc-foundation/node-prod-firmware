#ifndef __HW_LORA_H__
#define __HW_LORA_H__

#define MATCHSTICK_1_0a	0x010a
#define MATCHSTICK_2_0a	0x020a

#define HW_VERSION	MATCHSTICK_2_0a

#if HW_VERSION == MATCHSTICK_1_0a
#define HW_CONSOLE_UART_TX_PORT	HW_GPIO_PORT_4
#define HW_CONSOLE_UART_TX_PIN	HW_GPIO_PIN_5
#define HW_LORA_SPI_CLK_PORT	HW_GPIO_PORT_1
#define HW_LORA_SPI_CLK_PIN	HW_GPIO_PIN_0

#elif HW_VERSION == MATCHSTICK_2_0a
#define HW_CONSOLE_UART_TX_PORT	HW_GPIO_PORT_1
#define HW_CONSOLE_UART_TX_PIN	HW_GPIO_PIN_0
#define HW_LORA_SPI_CLK_PORT	HW_GPIO_PORT_1
#define HW_LORA_SPI_CLK_PIN	HW_GPIO_PIN_7

#define HW_SENSOR_MPU_I2C_ADDR	0x68
#define HW_SENSOR_MPU_INT_PORT	HW_GPIO_PORT_3
#define HW_SENSOR_MPU_INT_PIN	HW_GPIO_PIN_2
#define HW_SENSOR_I2C_SCL_PORT	HW_GPIO_PORT_1
#define HW_SENSOR_I2C_SCL_PIN	HW_GPIO_PIN_3
#define HW_SENSOR_I2C_SDA_PORT	HW_GPIO_PORT_0
#define HW_SENSOR_I2C_SDA_PIN	HW_GPIO_PIN_7

#else
#error Unsupported HW_VERSION
#endif

#define HW_CONSOLE_UART_RX_PORT	HW_GPIO_PORT_4
#define HW_CONSOLE_UART_RX_PIN	HW_GPIO_PIN_4

#define HW_PS_EN_PORT		HW_GPIO_PORT_2
#define HW_PS_EN_PIN		HW_GPIO_PIN_3

#define HW_LORA_SPI_NO		2

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
#define HW_SENSOR_BACKUP_PORT	HW_GPIO_PORT_0
#define HW_SENSOR_BACKUP_PIN	HW_GPIO_PIN_6
#define HW_IMU_BACKUP_PORT	HW_GPIO_PORT_2
#define HW_IMU_BACKUP_PIN	HW_GPIO_PIN_4

#define HW_SENSOR_UART_RX_PORT	HW_GPIO_PORT_4
#define HW_SENSOR_UART_RX_PIN	HW_GPIO_PIN_7
#define HW_SENSOR_UART_TX_PORT	HW_GPIO_PORT_3
#define HW_SENSOR_UART_TX_PIN	HW_GPIO_PIN_0

#define HW_USER_BTN_PORT	HW_GPIO_PORT_3
#define HW_USER_BTN_PIN		HW_GPIO_PIN_3

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
