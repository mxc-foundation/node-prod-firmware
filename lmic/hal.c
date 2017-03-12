#include "oslmic.h"
#include "hal.h"

#include <hw_watchdog.h>
#include <hw_wkup.h>

//#define DEBUG

#ifdef DEBUG
#define SET_HIGH(port, pin)	hw_gpio_set_active(port, pin)
#define SET_LOW(port, pin)	hw_gpio_set_inactive(port, pin)
#else
#define SET_HIGH(port, pin)
#define SET_LOW(port, pin)
#endif

static QueueHandle_t	lora_queue;

static void
wkup_intr_cb(void)
{
	BaseType_t	woken = 0;
	ostime_t	now;

	if (hw_gpio_get_pin_status(HAL_LORA_DIO0_PORT, HAL_LORA_DIO0_PIN) ||
	    hw_gpio_get_pin_status(HAL_LORA_DIO1_PORT, HAL_LORA_DIO1_PIN)) {
		now = hal_ticks();
		xQueueSendFromISR(lora_queue, &now, &woken);
		SET_LOW(4, 1);
		SET_LOW(4, 2);
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
	hw_wkup_register_interrupt(wkup_intr_cb, 3);
}

void
hal_init()
{
	lora_queue = xQueueCreate(5, sizeof(ostime_t));
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
	__disable_irq();
	hw_watchdog_gen_RST();
	hw_watchdog_set_pos_val(1);
	hw_watchdog_unfreeze();
	for (;;)
		__WFI();
}

#define LONGSLEEP (2 * (s4_t)(configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ))

static u4_t	waituntil;

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
	ostime_t	when;

	if (dt > LONGSLEEP) {

		// Timer precision is 64 ticks.  Sleep for 64 to
		// 128 ticks less than specified.
		// Wait for timer or WKUP_GPIO interrupt
		if (xQueueReceive(lora_queue, &when,
		    dt / (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) - 1))
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
		taskYIELD();
	}
}
