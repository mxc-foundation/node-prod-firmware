#include <stdio.h>

#include <console.h>
#include <hw_timer1.h>
#include <hw_trng.h>
#include <platform_devices.h>
#include <sys_power_mgr.h>
#include <sys_rtc.h>
#include <sys_watchdog.h>

#include "lmic/lmic.h"
#include "lmic/hal.h"

#define MORE_TASKS
#define hello

#define BARRIER()   __asm__ __volatile__ ("":::"memory")
//#define HAL_LORA_SPI HW_SPI2

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

static uart_device	console_uart;

void
print(char *s)
{
	ad_uart_write(console_uart, s, strlen(s));
}

#ifdef hello
static void
say_hi(osjob_t *job)
{
	static int	n;
	char		buf[128];

	snprintf(buf, sizeof(buf),
	    "Hello #%u @ %u (%lu), TIMER1 trigger %lu\r\n",
	    n++, os_getTimeSecs(), os_getTime(),
	    hw_timer1_get_trigger());
	print(buf);
	os_setTimedCallback(job, os_getTime() + sec2osticks(1), say_hi);
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

static void
periph_setup(void)
{
	hw_gpio_set_pin_function(1, 3, HW_GPIO_MODE_OUTPUT,
	    HW_GPIO_FUNC_UART_TX);
	hw_gpio_set_pin_function(2, 3, HW_GPIO_MODE_INPUT,
	    HW_GPIO_FUNC_UART_RX);
	spi_init();
	//hw_gpio_configure(gpio_cfg);
}

static void
main_task_func(void *param)
{
	char		buf[64];
	int		n;
	uint32_t	ticks, prescaled = 0, fine = 0;
	uint8_t		addr = 0, byte = 0;
	HW_SPI_FIFO	old_mode;
#ifdef hello
	osjob_t		job;
#endif

	console_uart = ad_uart_open(SERIAL1);
#ifdef hello
	say_hi(&job);
	os_runloop();
#endif
	ad_uart_write(console_uart, "hello\r\n", 7);
	for (;;) {
		/*
		old_mode = hw_spi_change_fifo_mode(HAL_LORA_SPI,
		    HW_SPI_FIFO_TX_ONLY);
		*/
#if 1
		old_mode = hw_spi_change_fifo_mode(HAL_LORA_SPI,
		    HW_SPI_FIFO_RX_TX);
		hal_pin_nss(0);
		hal_spi(addr);
		byte = hal_spi(0);
		hal_pin_nss(1);
		hw_spi_change_fifo_mode(HAL_LORA_SPI, old_mode);
#endif
		n = snprintf(buf, sizeof(buf), "%02x: %02x\r\n", addr, byte);
		ad_uart_write(console_uart, buf, n);
		//ticks = hal_ticks() / configTOCK_RATE_HZ;
		ticks = os_getTime();
		//HW_TIMER1_GET_INSTANT(prescaled, fine);
		n = snprintf(buf, sizeof(buf), "ticks: %u (%lu) [%lu, %lu]\r\n",
		    os_getTimeSecs(), ticks, prescaled, fine);
		ad_uart_write(console_uart, buf, n);
		hal_waitUntil(ticks + 1000);
		n = snprintf(buf, sizeof(buf), "ticks: %u (%lu) [%lu, %lu]\r\n",
		    os_getTimeSecs(), os_getTime(), prescaled, fine);
		ad_uart_write(console_uart, buf, n);
		LMIC_reset();
		addr = (addr + 1) & 0x7f;
	}
}

static void
spi_task_func(void *param)
{
	HW_SPI_FIFO	old_mode;

	for (;;) {
                //NVIC_DisableIRQ(SPI_IRQn);
		/*
		old_mode = hw_spi_change_fifo_mode(HAL_LORA_SPI,
		    HW_SPI_FIFO_TX_ONLY);
		*/
		old_mode = hw_spi_change_fifo_mode(HAL_LORA_SPI,
		    HW_SPI_FIFO_RX_TX);
		hw_spi_set_cs_low(HAL_LORA_SPI);
		hw_spi_fifo_write8(HAL_LORA_SPI, 0xa5);
		hw_spi_wait_while_busy(HAL_LORA_SPI);
		hw_spi_fifo_read8(HAL_LORA_SPI);
		hw_spi_set_cs_high(HAL_LORA_SPI);
		hw_spi_change_fifo_mode(HAL_LORA_SPI, old_mode);
	}
}

static OS_TASK xHandle;

static void
system_init(void *param)
{
#ifdef MORE_TASKS
	OS_TASK task_h;
#endif
#ifdef CONFIG_RETARGET
	extern void retarget_init(void);
#endif

	cm_sys_clk_init(sysclk_XTAL16M);
	cm_apb_set_clock_divider(apb_div1);
	cm_ahb_set_clock_divider(ahb_div1);
	cm_lp_clk_init();
	sys_watchdog_init();
	pm_system_init(periph_setup);
	ad_uart_init();
#ifdef MORE_TASKS
	OS_TASK_CREATE("main", main_task_func, NULL, 1000,
	    OS_TASK_PRIORITY_NORMAL, task_h);
	if(0)
	OS_TASK_CREATE("spi", spi_task_func, NULL, 1000,
	    OS_TASK_PRIORITY_NORMAL, task_h);
	OS_TASK_DELETE(xHandle);
#else
	main_task_func(0);
#endif
}

int
main()
{
	OS_TASK		task_h;

	cm_clk_init_low_level();
	OS_TASK_CREATE("init", system_init, 0,
	    configMINIMAL_STACK_SIZE * OS_STACK_WORD_SIZE,
	    OS_TASK_PRIORITY_HIGHEST, xHandle);
	vTaskStartScheduler();

#ifdef CONFIG_RETARGET
	extern void retarget_init(void);
#endif
	cm_sys_clk_init(sysclk_XTAL16M);
	cm_apb_set_clock_divider(apb_div1);
	cm_ahb_set_clock_divider(ahb_div1);
	cm_lp_clk_init();
	sys_watchdog_init();
	pm_system_init(periph_setup);
	ad_uart_init();

	//resource_init();
	OS_TASK_CREATE("main", main_task_func, NULL, 1000,
	    OS_TASK_PRIORITY_NORMAL, task_h);
	vTaskStartScheduler();
	//console_init(SERIAL1, 256);

	for (;;)
		;
	return 0;
}
