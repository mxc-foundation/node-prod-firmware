#include <stdio.h> //XXX
#include <hw_uart.h> //XXX
#include "oslmic.h"
#include "hal.h"

#include <hw_wkup.h>
#include <sys_power_mgr.h>
#include <sys_watchdog.h>

//#define DEBUG

#ifdef DEBUG
#define SET_HIGH(port, pin)	hw_gpio_set_active(port, pin)
#define SET_LOW(port, pin)	hw_gpio_set_inactive(port, pin)
#else
#define SET_HIGH(port, pin)
#define SET_LOW(port, pin)
#endif

PRIVILEGED_DATA static int8_t	wdog_id;
static QueueHandle_t		lora_queue;

static bool
ad_lmic_prepare_for_sleep(void)
{
	//extern unsigned long	_vStackTop[], _pvHeapStart[];

	//printf("%s [%lu]\r\n", __func__, (unsigned long)_vStackTop - (unsigned long)_pvHeapStart);
	hw_uart_write_buffer(HW_UART1, __func__, strlen(__func__));
	hw_uart_write_buffer(HW_UART1, "\r\n", 2);
	//printf("%s\r\n", __func__);
	sys_watchdog_notify(wdog_id); // XXX
	sys_watchdog_suspend(wdog_id); // XXX
	return true; // XXX
}

static void
ad_lmic_sleep_canceled(void)
{
	hw_uart_write_buffer(HW_UART1, __func__, strlen(__func__));
	hw_uart_write_buffer(HW_UART1, "\r\n", 2);
}

static void
ad_lmic_wake_up_ind(bool arg)
{
	hw_uart_write_buffer(HW_UART1, __func__, strlen(__func__));
	hw_uart_write_buffer(HW_UART1, arg ? "(1)\r\n" : "(0)\r\n", 5);
}

static void
ad_lmic_xtal16m_ready_ind(void)
{
	hw_uart_write_buffer(HW_UART1, __func__, strlen(__func__));
	hw_uart_write_buffer(HW_UART1, "\r\n", 2);
}

static const adapter_call_backs_t	ad_lmic_call_backs = {
	.ad_prepare_for_sleep		= ad_lmic_prepare_for_sleep,
	.ad_sleep_canceled		= ad_lmic_sleep_canceled,
	.ad_wake_up_ind			= ad_lmic_wake_up_ind,
	.ad_xtal16m_ready_ind		= ad_lmic_xtal16m_ready_ind,
	.ad_sleep_preparation_time	= 0,
};

static void
wkup_intr_cb(void)
{
	BaseType_t	woken = 0;
	ostime_t	now;

	if (hw_gpio_get_pin_status(HAL_LORA_DIO0_PORT, HAL_LORA_DIO0_PIN) ||
	    hw_gpio_get_pin_status(HAL_LORA_DIO1_PORT, HAL_LORA_DIO1_PIN)) {
		now = hal_ticks();
		xQueueSendFromISR(lora_queue, &now, &woken);
	}
	hw_wkup_reset_interrupt();
	portYIELD_FROM_ISR(woken);
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

void
hal_init()
{
	cm_wait_lp_clk_ready();
	wdog_id = sys_watchdog_register(false);
	sys_watchdog_notify(wdog_id);
	pm_register_adapter(&ad_lmic_call_backs);
	lora_queue = xQueueCreate(1, sizeof(ostime_t));
	wkup_init();
#ifdef DEBUG
	hw_gpio_set_pin_function(4, 0, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(4, 1, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(4, 2, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
#endif
}

void
hal_pin_rst(u1_t val)
{
	if (val == 2) {
		hw_gpio_set_pin_function(HAL_LORA_REST_PORT, HAL_LORA_REST_PIN,
		    HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO);
	} else {
		hw_gpio_set_pin_function(HAL_LORA_REST_PORT, HAL_LORA_REST_PIN,
		    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
		if (val) {
			hw_gpio_set_active(HAL_LORA_REST_PORT,
			    HAL_LORA_REST_PIN);
		} else {
			hw_gpio_set_inactive(HAL_LORA_REST_PORT,
			    HAL_LORA_REST_PIN);
		}
	}
}

u1_t
hal_spi(u1_t outval)
{
	hw_spi_wait_while_busy(HW_SPI2);
	hw_spi_fifo_write8(HW_SPI2, outval);
	hw_spi_wait_while_busy(HW_SPI2);
	return hw_spi_fifo_read8(HW_SPI2);
}

void
hal_failed()
{
	printf("hal failed\r\n");
	hw_cpm_reboot_system();
}

#define LONGSLEEP (2 * (s4_t)(configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ))

static u4_t	waituntil;

void
hal_resetWatchdog(void)
{
	sys_watchdog_notify(wdog_id);
}

u1_t
hal_checkTimer(u4_t targettime)
{
	s4_t	dt;

	dt = targettime - hal_ticks();
	if (dt < 5) {
		// Expiration time is nigh
		return 1;
	} else {
		// Set wake-up time for hal_sleep()
		waituntil = targettime;
		return 0;
	}
}

void
hal_setShortSleep()
{
	waituntil = hal_ticks() + sec2osticks(1);
}

void
hal_sleep()
{
	s4_t	dt = waituntil - hal_ticks();

	if (dt > LONGSLEEP) {
		BaseType_t	ret;
		ostime_t	when;

		if (dt >= 0x10000)
			sys_watchdog_suspend(wdog_id);
		// Timer precision is 64 ticks.  Sleep for 64 to
		// 128 ticks less than specified.
		// Wait for timer or WKUP_GPIO interrupt
		ret = xQueueReceive(lora_queue, &when,
		    dt / (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) - 1);
		sys_watchdog_notify_and_resume(wdog_id);
		if (ret)
			radio_irq_handler(when);
	} else {
		// Busy-wait
		hal_waitUntil(waituntil - 5);
	}
}

void
hal_waitUntil(u4_t time)
{
	ostime_t	when;

	while ((s4_t)(time - hal_ticks()) > 0) {
		if (xQueueReceive(lora_queue, &when, 0)) {
			radio_irq_handler(when);
			break;
		}
	}
}
