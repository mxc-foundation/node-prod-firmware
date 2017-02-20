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

//#define MORE_TASKS
//#define hello
#define join

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

//static uart_device	console_uart;

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

	printf("Hello #%u @ %u (%lu), TIMER1 trigger %lu\r\n",
	    n++, os_getTimeSecs(), os_getTime(),
	    hw_timer1_get_trigger());
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
	uint32_t	ticks, prescaled = 0, fine = 0;
	uint8_t		addr = 0, byte = 0;
	HW_SPI_FIFO	old_mode;
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
#ifdef hello
	osjob_t		job;
#endif
#ifdef join
	extern int	join_main(void);
#endif

	//console_uart = ad_uart_open(SERIAL1);
	//hw_uart_init_ex(HW_UART1, &uart_cfg);
	hw_uart_init(HW_UART1, &uart_cfg);
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
#if 1
		old_mode = hw_spi_change_fifo_mode(HAL_LORA_SPI,
		    HW_SPI_FIFO_RX_TX);
		hal_pin_nss(0);
		hal_spi(addr);
		byte = hal_spi(0);
		hal_pin_nss(1);
		hw_spi_change_fifo_mode(HAL_LORA_SPI, old_mode);
#endif
		printf("%02x: %02x\r\n", addr, byte);
		//ticks = hal_ticks() / configTOCK_RATE_HZ;
		ticks = os_getTime();
		//HW_TIMER1_GET_INSTANT(prescaled, fine);
		printf("ticks: %u (%lu) [%lu, %lu]\r\n",
		    os_getTimeSecs(), ticks, prescaled, fine);
		hal_waitUntil(ticks + 1000);
		printf("ticks: %u (%lu) [%lu, %lu]\r\n",
		    os_getTimeSecs(), os_getTime(), prescaled, fine);
		LMIC_reset();
		addr = (addr + 1) & 0x7f;
	}
}

#ifdef MORE_TASKS
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
#endif

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
	hw_cpm_set_pclk_div(apb_div1);
	//cm_apb_set_clock_divider(apb_div1);
	//cm_ahb_set_clock_divider(ahb_div1);
	cm_lp_clk_init();
	sys_watchdog_init();
	pm_system_init(periph_setup);
	//ad_uart_init();
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

/* static */ void
cm_clk_init_low_level_x(void)
{
	uint32_t	reg;

	NVIC_ClearPendingIRQ(XTAL16RDY_IRQn);
	NVIC_EnableIRQ(XTAL16RDY_IRQn);
	//hw_cpm_set_divn(false);
	reg = CRG_TOP->CLK_CTRL_REG;
	REG_SET_FIELD(CRG_TOP, CLK_CTRL_REG, DIVN_XTAL32M_MODE, reg, 0);
	REG_SET_FIELD(CRG_TOP, CLK_CTRL_REG, XTAL32M_MODE, reg, 0);
	CRG_TOP->CLK_CTRL_REG = reg;
	CRG_TOP->DIVN_SYNC_REG = 0;
	// hw_cpm_enable_rc32k
	REG_SET_BIT(CRG_TOP, CLK_32K_REG, RC32K_ENABLE);
	// hw_cpm_lp_set_rc32k
	ASSERT_WARNING(REG_GETF(CRG_TOP, CLK_32K_REG, RC32K_ENABLE) == 1);
	REG_SETF(CRG_TOP, CLK_CTRL_REG, CLK32K_SOURCE, LP_CLK_IS_RC32K);
	// hw_cpm_set_xtal16m_settling_time(dg_configXTAL16_SETTLE_TIME_RC32K);
	CRG_TOP->XTALRDY_CTRL_REG = dg_configXTAL16_SETTLE_TIME_RC32K;
	// hw_cpm_enable_xtal16m
	REG_CLR_BIT(CRG_TOP, CLK_CTRL_REG, XTAL16M_DISABLE);
	GPIO->P20_MODE_REG = 0x26;
	GPIO->P21_MODE_REG = 0x26;
	reg = CRG_TOP->CLK_32K_REG;
	REG_SET_FIELD(CRG_TOP, CLK_32K_REG, XTAL32K_CUR, reg, 5);
	REG_SET_FIELD(CRG_TOP, CLK_32K_REG, XTAL32K_RBIAS, reg, 3);
	REG_SET_FIELD(CRG_TOP, CLK_32K_REG, XTAL32K_DISABLE_AMPREG, reg, 0);
	CRG_TOP->CLK_32K_REG = reg;
	REG_SET_BIT(CRG_TOP, CLK_32K_REG, XTAL32K_ENABLE);
	cm_check_xtal_startup();
	CRG_TOP->SLEEP_TIMER_REG = 3000;
}

int
main()
{
	//OS_TASK		task_h;

	cm_clk_init_low_level_x();
	//system_init(0);
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
	//sys_watchdog_init();
	pm_system_init(periph_setup);
	ad_uart_init();

	//resource_init();
	/*
	OS_TASK_CREATE("main", main_task_func, NULL, 1000,
	    OS_TASK_PRIORITY_NORMAL, task_h);
	*/
	main_task_func(0);
	vTaskStartScheduler();
	//console_init(SERIAL1, 256);

	for (;;)
		;
	return 0;
}
