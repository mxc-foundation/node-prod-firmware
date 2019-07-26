#ifndef __HW_LORA_H__
#define __HW_LORA_H__

#ifndef HW_VERSION
#define HW_VERSION  HW_SOM_1_2
#endif

#define HW_SOM_1_0		      0x1100
#define HW_SOM_1_2		      0x1120

#define HW_IS_SOM		      ((HW_VERSION & 0xf000) == 0x1000)

#if HW_IS_SOM

#if HW_VERSION == HW_SOM_1_0

#define HW_VER_STRING		"DevKit 1.0"

#elif HW_VERSION == HW_SOM_1_2

#define HW_VER_STRING		"DevKit 1.2"

#define FEATURE_LED_RGB_REV
#define FEATURE_SENSOR_GPS

#define HW_SENSOR_UART_RX_PORT	  HW_GPIO_PORT_3
#define HW_SENSOR_UART_RX_PIN	    HW_GPIO_PIN_3
#define HW_SENSOR_UART_TX_PORT	  HW_GPIO_PORT_2
#define HW_SENSOR_UART_TX_PIN	    HW_GPIO_PIN_3

#else
#error Unsupported SOM version
#endif

#define FEATURE_BATTERY
#define FEATURE_I2C
#define FEATURE_LED_RGB
#define FEATURE_POWER_SUPPLY
#define FEATURE_USER_BUTTON
#define FEATURE_SENSOR
#define FEATURE_SENSOR_TEMP

// uncomment one of the temperature sensors below.
#define FEATURE_SENSOR_TEMP_PCT2075
//#define FEATURE_SENSOR_TEMP_INTERNAL

// uncomment below define if you want to use grove digital light sensor.
//#define FEATURE_SENSOR_LIGHT

// define initial sleep mode according to your power needs.
#define INITIAL_SLEEP_MODE	      pm_mode_extended_sleep

#define HW_PS_EN_PORT	            HW_GPIO_PORT_3
#define HW_PS_EN_PIN		          HW_GPIO_PIN_4

#define HW_LORA_SPI_DI_PORT	      HW_GPIO_PORT_3
#define HW_LORA_SPI_DI_PIN	      HW_GPIO_PIN_7
#define HW_LORA_SPI_DO_PORT	      HW_GPIO_PORT_3
#define HW_LORA_SPI_DO_PIN	      HW_GPIO_PIN_6
#define HW_LORA_SPI_CS_PORT	      HW_GPIO_PORT_3
#define HW_LORA_SPI_CS_PIN	      HW_GPIO_PIN_5
#define HW_LORA_SPI_CLK_PORT	    HW_GPIO_PORT_4
#define HW_LORA_SPI_CLK_PIN	      HW_GPIO_PIN_0
#define HW_LORA_RX_PORT		        HW_GPIO_PORT_4
#define HW_LORA_RX_PIN		        HW_GPIO_PIN_7
#define HW_LORA_TX_PORT		        HW_GPIO_PORT_3
#define HW_LORA_TX_PIN		        HW_GPIO_PIN_0
#define HW_LORA_REST_PORT	        HW_GPIO_PORT_1
#define HW_LORA_REST_PIN	        HW_GPIO_PIN_0
#define HW_LORA_DIO0_PORT	        HW_GPIO_PORT_1
#define HW_LORA_DIO0_PIN	        HW_GPIO_PIN_5
#define HW_LORA_DIO1_PORT	        HW_GPIO_PORT_1
#define HW_LORA_DIO1_PIN	        HW_GPIO_PIN_2
#define HW_LORA_DIO2_PORT	        HW_GPIO_PORT_1
#define HW_LORA_DIO2_PIN	        HW_GPIO_PIN_7

#define HW_CONSOLE_UART_TX_PORT	  HW_GPIO_PORT_1
#define HW_CONSOLE_UART_TX_PIN	  HW_GPIO_PIN_4
#define HW_CONSOLE_UART_RX_PORT	  HW_GPIO_PORT_1
#define HW_CONSOLE_UART_RX_PIN	  HW_GPIO_PIN_6

#define HW_USER_BTN_PORT	        HW_GPIO_PORT_3
#define HW_USER_BTN_PIN		        HW_GPIO_PIN_2
#define HW_USER_BTN_ACTIVE	      HW_WKUP_PIN_STATE_HIGH

#define HW_IOX_I2C_ADDR		              0x20
#define HW_SENSOR_TEMP_I2C_ADDR	        0x49
#define HW_SENSOR_GROVE_LIGHT_I2C_ADDR	0x29
#define HW_I2C_SCL_PORT		        HW_GPIO_PORT_4
#define HW_I2C_SCL_PIN		        HW_GPIO_PIN_3
#define HW_I2C_SDA_PORT		        HW_GPIO_PORT_4
#define HW_I2C_SDA_PIN		        HW_GPIO_PIN_2

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
