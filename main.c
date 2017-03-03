#include <stdio.h>

#include <console.h>
#include <hw_cpm.h>
#include <hw_otpc.h>
#include <hw_qspi.h>
#include <hw_timer1.h>
#include <hw_trng.h>
#include <hw_uart.h>
#include <hw_watchdog.h>
#include <hw_wkup.h>

#include "lmic/lmic.h"
#include "lmic/hal.h"
#ifdef OS_BAREMETAL
#include "rtc.h"
#endif

#define hello
#define join

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

#ifndef OS_FREERTOS
PRIVILEGED_DATA sys_clk_t	cm_sysclk = sysclk_XTAL16M;
PRIVILEGED_DATA ahb_div_t	cm_ahbclk = ahb_div1;
#endif

#ifdef OS_FREERTOS
static OS_TASK xHandle;

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

	os_setTimedCallback(job, os_getTime() + sec2osticks(5), say_hi);
	printf("Hello #%u @ %u (%#010lx), TIMER1 trigger %#010lx\r\n",
	    n++, os_getTimeSecs(), os_getTime(), hw_timer1_get_trigger());
#if 0
	if (n == 5)
		hal_failed();
#endif
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
wkup_intr_cb(void)
{
	uint8_t	dio = -1;

	if (hw_gpio_get_pin_status(HAL_LORA_DIO0_PORT, HAL_LORA_DIO0_PIN))
		dio = 0;
	else if (hw_gpio_get_pin_status(HAL_LORA_DIO1_PORT, HAL_LORA_DIO1_PIN))
		dio = 1;
	else if (hw_gpio_get_pin_status(HAL_LORA_DIO2_PORT, HAL_LORA_DIO2_PIN))
		dio = 2;
	else
		goto fail;
	radio_irq_handler(dio);
fail:
	hw_wkup_reset_interrupt();
}

static void
wkup_init(void)
{
	hw_wkup_init(NULL);
	hw_wkup_set_counter_threshold(1);
	hw_gpio_set_pin_function(HAL_LORA_DIO0_PORT, HAL_LORA_DIO0_PIN,
	    HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(HAL_LORA_DIO1_PORT, HAL_LORA_DIO1_PIN,
	    HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(HAL_LORA_DIO2_PORT, HAL_LORA_DIO2_PIN,
	    HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO);
	hw_wkup_configure_pin(HAL_LORA_DIO0_PORT, HAL_LORA_DIO0_PIN, true,
	    HW_WKUP_PIN_STATE_HIGH);
	hw_wkup_configure_pin(HAL_LORA_DIO1_PORT, HAL_LORA_DIO1_PIN, true,
	    HW_WKUP_PIN_STATE_HIGH);
	hw_wkup_configure_pin(HAL_LORA_DIO2_PORT, HAL_LORA_DIO2_PIN, true,
	    HW_WKUP_PIN_STATE_HIGH);
	hw_wkup_register_interrupt(wkup_intr_cb, 1);
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
#ifdef OS_BAREMETAL
	rtc_init();
#endif
	wkup_init();
}

static void
main_task_func(void *param)
{
	uint8_t		addr = 0, byte = 0;
#ifdef hello
	osjob_t		job;
#endif
#ifdef join
	extern int	join_main(void);
#endif

	cm_sys_clk_init(sysclk_XTAL16M);
	cm_apb_set_clock_divider(apb_div1);
	cm_ahb_set_clock_divider(ahb_div1);
	cm_lp_clk_init();
	//sys_watchdog_init();
#if dg_configUSE_WDOG
	// Register the Idle task first.
	idle_task_wdog_id = sys_watchdog_register(false);
	ASSERT_WARNING(idle_task_wdog_id != -1);
	sys_watchdog_configure_idle_id(idle_task_wdog_id);
#endif
	os_init();
#ifdef hello
	say_hi(&job);
#ifndef join
	os_runloop();
#endif
#endif
#ifdef join
	join_main();
#endif
	printf("hello\r\n");
	for (;;) {
		hal_pin_nss(0);
		hal_spi(addr);
		byte = hal_spi(0);
		hal_pin_nss(1);
		printf("%02x: %02x\r\n", addr, byte);
		addr = (addr + 1) & 0x7f;
	}
}

int
main()
{
#ifdef OS_FREERTOS
	cm_clk_init_low_level();
#endif
	periph_setup();
	printf("*** BARE METAL ***\r\n");

	OS_TASK_CREATE("sysinit", main_task_func, (void *)0,
	    512 * OS_STACK_WORD_SIZE,
	    OS_TASK_PRIORITY_HIGHEST, xHandle);
	//main_task_func(0);

	vTaskStartScheduler();
	for (;;)
		;
	return 0;
}

void debug_event (int ev) {
	static const char* evnames[] = {
		[EV_SCAN_TIMEOUT]   = "SCAN_TIMEOUT",
		[EV_BEACON_FOUND]   = "BEACON_FOUND",
		[EV_BEACON_MISSED]  = "BEACON_MISSED",
		[EV_BEACON_TRACKED] = "BEACON_TRACKED",
		[EV_JOINING]        = "JOINING",
		[EV_JOINED]         = "JOINED",
		[EV_RFU1]           = "RFU1",
		[EV_JOIN_FAILED]    = "JOIN_FAILED",
		[EV_REJOIN_FAILED]  = "REJOIN_FAILED",
		[EV_TXCOMPLETE]     = "TXCOMPLETE",
		[EV_LOST_TSYNC]     = "LOST_TSYNC",
		[EV_RESET]          = "RESET",
		[EV_RXCOMPLETE]     = "RXCOMPLETE",
		[EV_LINK_DEAD]      = "LINK_DEAD",
		[EV_LINK_ALIVE]     = "LINK_ALIVE",
		[EV_SCAN_FOUND]     = "SCAN_FOUND",
		[EV_TXSTART]        = "EV_TXSTART",
	};
	printf("%s\r\n", (ev < sizeof(evnames)/sizeof(evnames[0])) ? evnames[ev] : "EV_UNKNOWN" );
}
