#include <stdio.h>

#include <console.h>
#include <hw_breath.h>
#include <hw_led.h>
#include <hw_timer0.h>
#include <hw_timer2.h>
#include <hw_trng.h>
#include <hw_uart.h>
#include <resmgmt.h>
#include <sys_power_mgr.h>
#include <sys_watchdog.h>

#include "lmic/lmic.h"
#include "proto.h"

#define CLOCK_DEBUG

#define BARRIER()   __asm__ __volatile__ ("":::"memory")

#if HAL_LORA_SPI_NO == 1
#define HAL_LORA_GPIO_FUNC_SPI_CLK	HW_GPIO_FUNC_SPI_CLK
#define HAL_LORA_GPIO_FUNC_SPI_DI	HW_GPIO_FUNC_SPI_DI
#define HAL_LORA_GPIO_FUNC_SPI_DO	HW_GPIO_FUNC_SPI_DO
#elif HAL_LORA_SPI_NO == 2
#define HAL_LORA_GPIO_FUNC_SPI_CLK	HW_GPIO_FUNC_SPI2_CLK
#define HAL_LORA_GPIO_FUNC_SPI_DI	HW_GPIO_FUNC_SPI2_DI
#define HAL_LORA_GPIO_FUNC_SPI_DO	HW_GPIO_FUNC_SPI2_DO
#else
#error "Invalid HAL_LORA_SPI_NO definition"
#endif

extern int	_write(int fd, char *ptr, int len);

#ifdef CONFIG_RETARGET
extern void	retarget_init(void);
#endif

#if dg_configUSE_WDOG
INITIALISED_PRIVILEGED_DATA int8_t	idle_task_wdog_id = -1;
#endif

static OS_TASK lmic_handle;

void
vApplicationMallocFailedHook(void)
{
	printf("malloc\r\n");
	hal_failed();
}

void
vApplicationStackOverflowHook(void)
{
	printf("stack\r\n");
	hal_failed();
}

#if dg_configUSE_WDOG
void
vApplicationIdleHook(void)
{
#ifdef CLOCK_DEBUG
	_write(1, ".", 1);
#endif
	sys_watchdog_notify(idle_task_wdog_id);
}
#endif

#ifdef CONFIG_CUSTOM_PRINT
int
_write(int fd, char *ptr, int len)
{
	hw_uart_write_buffer(HW_UART1, ptr, len);
	(void)fd;
	return len;
}
#endif

static void
spi_init(void)
{
	spi_config	cfg = {
		{HAL_LORA_SPI_CS_PORT, HAL_LORA_SPI_CS_PIN},
		HW_SPI_WORD_8BIT,
		HW_SPI_MODE_MASTER,
		HW_SPI_POL_LOW,
		HW_SPI_PHA_MODE_0,
		HW_SPI_MINT_DISABLE,
		HW_SPI_FREQ_DIV_2,
		HW_SPI_FIFO_RX_TX,
		0,
		0,
		0,
		0,
	};

	hw_trng_enable(NULL);
	hw_gpio_set_pin_function(HAL_LORA_SPI_CLK_PORT, HAL_LORA_SPI_CLK_PIN,
	    HW_GPIO_MODE_OUTPUT, HAL_LORA_GPIO_FUNC_SPI_CLK);
	hw_gpio_set_pin_function(HAL_LORA_SPI_DI_PORT,  HAL_LORA_SPI_DI_PIN,
	    HW_GPIO_MODE_INPUT,  HAL_LORA_GPIO_FUNC_SPI_DI);
	hw_gpio_set_pin_function(HAL_LORA_SPI_DO_PORT,  HAL_LORA_SPI_DO_PIN,
	    HW_GPIO_MODE_OUTPUT, HAL_LORA_GPIO_FUNC_SPI_DO);
	hw_gpio_set_pin_function(HAL_LORA_SPI_CS_PORT,  HAL_LORA_SPI_CS_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(HAL_LORA_RX_PORT,      HAL_LORA_RX_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(HAL_LORA_TX_PORT,      HAL_LORA_TX_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_spi_init(HAL_LORA_SPI, &cfg);
}

#if 0
static void
sensor_init(void)
{
#define SENSOR_PORT	0
#define SENSOR_PIN	7
	hw_gpio_set_pin_function(SENSOR_PORT, SENSOR_PIN,
	    HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_ADC);
}

static int
sensor_get(void)
{
}
#endif

#ifdef CONFIG_RTT
#define uart_init()
#else
static void
uart_init(void)
{
	uart_config	uart_cfg = {
		HW_UART_BAUDRATE_115200,
		HW_UART_DATABITS_8,
		HW_UART_PARITY_NONE,
		HW_UART_STOPBITS_1,
		0,
		0,
		1,
		HW_DMA_CHANNEL_1,
		HW_DMA_CHANNEL_0,
	};

	hw_gpio_set_pin_function(4, 5, HW_GPIO_MODE_OUTPUT,
	    HW_GPIO_FUNC_UART_TX);
	hw_gpio_set_pin_function(4, 4, HW_GPIO_MODE_INPUT,
	    HW_GPIO_FUNC_UART_RX);
	hw_uart_init(HW_UART1, &uart_cfg);
}
#endif

#ifdef CLOCK_DEBUG
static void
timer0_cb(void)
{
	static bool	on;

	on = !on;
	hw_led_enable_led3(on);
}
#endif

static void
led_init(void)
{
	breath_config	bcfg = {
		.dc_min		= 0,
		.dc_max		= 0xff,
		.dc_step	= 0x20,
		.freq_div	= 0xff,
		.polarity	= HW_BREATH_PWM_POL_POS,
	};
#ifdef CLOCK_DEBUG
	timer0_config	t0cfg = {
		.clk_src	= HW_TIMER0_CLK_SRC_SLOW,
		.on_reload	= 0x1fff,
	};
#endif
	timer2_config	t2cfg = {
		.pwm4_start	= 0,
		.pwm4_end	= 0xffff,
	};

#ifdef CLOCK_DEBUG
	hw_timer0_init(&t0cfg);
	hw_timer0_register_int(timer0_cb);
	hw_timer0_enable();
#endif
	hw_timer2_init(&t2cfg);
	hw_timer2_enable();
	hw_breath_init(&bcfg);
	hw_led_set_led3_src(HW_LED_SRC3_PWM4);
	hw_led_enable_led3(true);
	hw_breath_enable();
}

static void
periph_setup(void)
{
	uart_init();
	spi_init();
	led_init();
}

static void
sysinit_task_func(void *param)
{
	cm_sys_clk_init(sysclk_XTAL16M);
	cm_apb_set_clock_divider(apb_div1);
	cm_ahb_set_clock_divider(ahb_div1);
	cm_lp_clk_init();
#if dg_configUSE_WDOG
	sys_watchdog_init();
	// Register the Idle task first.
	idle_task_wdog_id = sys_watchdog_register(false);
	ASSERT_WARNING(idle_task_wdog_id != -1);
	sys_watchdog_configure_idle_id(idle_task_wdog_id);
#ifdef CLOCK_DEBUG
	sys_watchdog_suspend(idle_task_wdog_id);
#endif
#endif
	cm_sys_clk_set(sysclk_XTAL16M);
	pm_system_init(periph_setup);
	printf("*** FreeRTOS ***\r\n");
	resource_init();
#ifdef CONFIG_RETARGET
	retarget_init();
#endif
	pm_set_wakeup_mode(true);
	//pm_set_sleep_mode(pm_mode_extended_sleep); //XXX
	pm_set_sleep_mode(pm_mode_idle); //XXX
	//pm_set_sleep_mode(pm_mode_active); //XXX
	//pm_stay_alive(); // XXX
	OS_TASK_CREATE("LoRa & LMiC", lora_task_func, (void *)0,
	    2048, OS_TASK_PRIORITY_NORMAL, lmic_handle);
	OS_TASK_DELETE(OS_GET_CURRENT_TASK());
}

int
main()
{
	OS_TASK	handle;

	cm_clk_init_low_level();
	OS_TASK_CREATE("sysinit", sysinit_task_func, (void *)0,
	    1024, OS_TASK_PRIORITY_HIGHEST, handle);
	vTaskStartScheduler();
	for (;;)
		;
	return 0;
}
