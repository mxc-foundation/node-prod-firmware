#include "oslmic.h"
#include "hal.h"

#include <hw_cpm.h>
#include <hw_timer1.h>
#include <hw_watchdog.h>
#include <hw_wkup.h>

static QueueHandle_t	lora_queue;

static int
handle_dio(uint8_t dio)
{
	BaseType_t      woken = 0;
	uint8_t         dummy = 1;

	radio_irq_handler(dio);
	xQueueSendFromISR(lora_queue, &dummy, &woken);
	return woken;
}

static void
wkup_intr_cb(void)
{
	int     woken = 0;

	if (hw_gpio_get_pin_status(HAL_LORA_DIO0_PORT, HAL_LORA_DIO0_PIN))
		woken = handle_dio(0);
	else if (hw_gpio_get_pin_status(HAL_LORA_DIO1_PORT, HAL_LORA_DIO1_PIN))
		woken = handle_dio(1);
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

//#define DEBUG

#ifdef DEBUG
#define SET_HIGH(port, pin)	hw_gpio_set_active(port, pin)
#define SET_LOW(port, pin)	hw_gpio_set_inactive(port, pin)
#define debug(...)		printf(__VA_ARGS__)
#else
#define SET_HIGH(port, pin)
#define SET_LOW(port, pin)
#define debug(...)
#endif

void
hal_init()
{
	lora_queue = xQueueCreate(5, sizeof(uint8_t));
	wkup_init();
#ifdef DEBUG
	hw_gpio_set_pin_function(1, 0, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(4, 2, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(0, 7, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
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
	hw_watchdog_unfreeze();
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
	uint8_t	dummy;

	if (dt > LONGSLEEP) {
		// Timer precision is 64 ticks.  Sleep for 64 to
		// 128 ticks less than specified.
		// Wait for timer or WKUP_GPIO interrupt
		debug("sleep %ld (%ld s)\r\n", dt / (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) - 1, dt / configSYSTICK_CLOCK_HZ);
		xQueueReceive(lora_queue, &dummy,
		    dt / (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) - 1);
	} else {
		// Busy-wait
		debug("wait %ld @ %ld\r\n", waituntil - 5, ticks);
		hal_waitUntil(waituntil - 5);
	}
}

void
hal_waitUntil(u4_t time)
{
	uint8_t	dummy;

	SET_HIGH(4, 2);
	while ((s4_t)(time - hal_ticks()) > 0) {
		if (xQueueReceive(lora_queue, &dummy, 0)) {
			debug("intr\r\n");
			break;
		}
	}
	SET_LOW(4, 2);
}
