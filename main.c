#include <stdio.h>

#include <console.h>
#include <hw_uart.h>
#include <resmgmt.h>
#include <sys_power_mgr.h>
#include <sys_watchdog.h>

#include "hw.h"
#include "led.h"
#include "lora.h"
#include "lmic/lmic.h"

#define BARRIER()   __asm__ __volatile__ ("":::"memory")

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

static void
periph_setup(void)
{
	uart_init();
	led_init();
	hal_periph_init();
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
#endif
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
	cm_sys_clk_set(sysclk_XTAL16M);
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
