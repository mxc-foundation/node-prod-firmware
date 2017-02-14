#include <console.h>
#include <hw_gpio.h>
#include <hw_led.h>
#include <platform_devices.h>
#include <sys_power_mgr.h>
#include <sys_watchdog.h>

#define MORE_TASKS

#define BARRIER()   __asm__ __volatile__ ("":::"memory")

static void
spi_init(void)
{
	spi_config	cfg = {
		{3, 2},
		HW_SPI_WORD_8BIT,
		HW_SPI_MODE_MASTER,
		HW_SPI_POL_LOW,
		HW_SPI_PHA_MODE_0,
		HW_SPI_MINT_DISABLE,
		HW_SPI_FREQ_DIV_8,
		HW_SPI_FIFO_RX_TX,
		0,
		0,
		0,
		0,
	};

	hw_gpio_set_pin_function(3, 6, HW_GPIO_MODE_OUTPUT,
	    HW_GPIO_FUNC_SPI_CLK);
	hw_gpio_set_pin_function(3, 4, HW_GPIO_MODE_INPUT,
	    HW_GPIO_FUNC_SPI_DI);
	hw_gpio_set_pin_function(3, 3, HW_GPIO_MODE_OUTPUT,
	    HW_GPIO_FUNC_SPI_DO);
	hw_gpio_set_pin_function(cfg.cs_pad.port, cfg.cs_pad.pin,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_spi_init(HW_SPI1, &cfg);
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
	uart_device	dev;
	HW_SPI_FIFO	old_mode;

	dev = ad_uart_open(SERIAL1);
	ad_uart_write(dev, "hello\r\n", 7);
	for (;;) {
		ad_uart_write(dev, "hi\r\n", 4);

                //NVIC_DisableIRQ(SPI_IRQn);
		old_mode = hw_spi_change_fifo_mode(HW_SPI1,
		    HW_SPI_FIFO_TX_ONLY);
		hw_spi_set_cs_low(HW_SPI1);
		hw_spi_fifo_write8(HW_SPI1, 0xa5);
		while (hw_spi_is_busy(HW_SPI1))
			BARRIER();
		hw_spi_set_cs_high(HW_SPI1);
		hw_spi_change_fifo_mode(HW_SPI1, old_mode);
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
