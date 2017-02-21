#include <stdio.h>

#include <console.h>
#include <hw_cpm.h>
#include <hw_otpc.h>
#include <hw_qspi.h>
#include <hw_timer1.h>
#include <hw_trng.h>
#include <hw_uart.h>
#include <hw_watchdog.h>

#include "lmic/lmic.h"
#include "lmic/hal.h"
#include "rtc.h"

#define hello
//#define join

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

int
_write(int fd, char *ptr, int len)
{
	(void)fd;
	hw_uart_write_buffer(HW_UART1, ptr, len);
	return len;
}

#ifdef hello
static void
say_hi(osjob_t *job)
{
	static int	n;

	printf("Hello #%u @ %u (%#010lx), TIMER1 trigger %#010lx\r\n",
	    n++, os_getTimeSecs(), os_getTime(), hw_timer1_get_trigger());
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

	hw_gpio_set_pin_function(1, 3, HW_GPIO_MODE_OUTPUT,
	    HW_GPIO_FUNC_UART_TX);
	hw_gpio_set_pin_function(2, 3, HW_GPIO_MODE_INPUT,
	    HW_GPIO_FUNC_UART_RX);
	hw_uart_init(HW_UART1, &uart_cfg);
	spi_init();
	rtc_init();
}

static void
main_task_func(void *param)
{
	//uint32_t	ticks, prescaled = 0, fine = 0;
	uint8_t		addr = 0, byte = 0;
	//HW_SPI_FIFO	old_mode;
#ifdef hello
	osjob_t		job;
#endif
#ifdef join
	extern int	join_main(void);
#endif

	//console_uart = ad_uart_open(SERIAL1);
	//hw_uart_init_ex(HW_UART1, &uart_cfg);
	//hw_uart_init(HW_UART1, &uart_cfg);
	printf("init\r\n");
#ifdef join
	join_main();
#endif
#ifdef hello
	say_hi(&job);
	os_runloop();
#endif
	printf("hello\r\n");
	for (;;) {
		/*
		old_mode = hw_spi_change_fifo_mode(HAL_LORA_SPI,
		    HW_SPI_FIFO_TX_ONLY);
		*/
		printf("%02x: %02x\r\n", addr, byte);
		addr = (addr + 1) & 0x7f;
	}
}

PRIVILEGED_DATA sys_clk_t cm_sysclk;
PRIVILEGED_DATA ahb_div_t cm_ahbclk;

int
main()
{
	//OS_TASK		task_h;
	uint32_t	dummy;

	periph_setup();
        hw_timer1_lp_clk_init();
        //HW_TIMER1_SET_TRIGGER(16 - 1, dummy); // Set initial reload value
	(void)dummy;
	printf("bare metal\r\n");
        hw_timer1_int_enable();                         // Enable interrupt
        hw_timer1_enable();                             // Start running

#ifdef CONFIG_RETARGET
	extern void retarget_init(void);
#endif

	main_task_func(0);

	for (;;)
		;
	return 0;
}
