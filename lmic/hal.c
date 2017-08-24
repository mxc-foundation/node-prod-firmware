#include <unistd.h>

#include <hw_wkup.h>
#include <sys_power_mgr.h>
#include <sys_watchdog.h>

#include "oslmic.h"
#include "hal.h"
#include "hw/button.h"
#include "hw/cons.h"
#include "hw/power.h"
#include "sensor/sensor.h"

#define WATCHDOG_ALWAYS_ON

#define EV_LORA_DIO	0
#define EV_BTN_PRESS	1
#define EV_CONS_RX	2
struct event {
	uint8_t		ev;
	uint32_t	data;
};

PRIVILEGED_DATA static int8_t		wdog_id;
PRIVILEGED_DATA static QueueHandle_t	lora_queue;

void
hal_uart_rx(void)
{
	BaseType_t	woken = 0;
	struct event	ev = {
		.ev	= EV_CONS_RX,
		.data	= 0,
	};

	if (lora_queue) {
		xQueueSendFromISR(lora_queue, &ev, &woken);
		portYIELD_FROM_ISR(woken);
	}
}

static void
wkup_intr_cb(void)
{
	BaseType_t	woken = 0;
	struct event	ev;

	ev.data = hal_ticks_fromISR();
	if (hw_gpio_get_pin_status(HW_LORA_DIO0_PORT, HW_LORA_DIO0_PIN) ||
	    hw_gpio_get_pin_status(HW_LORA_DIO1_PORT, HW_LORA_DIO1_PIN)) {
		ev.ev = EV_LORA_DIO;
	} else {
		ev.ev = EV_BTN_PRESS;
	}
	if (lora_queue)
		xQueueSendFromISR(lora_queue, &ev, &woken);
	hw_wkup_reset_interrupt();
	if (lora_queue)
		portYIELD_FROM_ISR(woken);
}

static void
wkup_init(void)
{
	hw_wkup_init(NULL);
	hw_wkup_set_counter_threshold(1);
	hw_wkup_configure_pin(HW_LORA_DIO0_PORT, HW_LORA_DIO0_PIN, true,
	    HW_WKUP_PIN_STATE_HIGH);
	hw_wkup_configure_pin(HW_LORA_DIO1_PORT, HW_LORA_DIO1_PIN, true,
	    HW_WKUP_PIN_STATE_HIGH);
	hw_wkup_configure_pin(HW_LORA_DIO2_PORT, HW_LORA_DIO2_PIN, true,
	    HW_WKUP_PIN_STATE_HIGH);
	hw_wkup_configure_pin(HW_USER_BTN_PORT,  HW_USER_BTN_PIN,  true,
	    HW_WKUP_PIN_STATE_LOW);
	hw_wkup_register_interrupt(wkup_intr_cb, 1);
}

static void
hal_lora_init(void)
{
	spi_config	cfg = {
		.cs_pad		= {
			.port	= HW_LORA_SPI_CS_PORT,
			.pin	= HW_LORA_SPI_CS_PIN,
		},
		.word_mode	= HW_SPI_WORD_8BIT,
		.smn_role	= HW_SPI_MODE_MASTER,
		.polarity_mode	= HW_SPI_POL_LOW,
		.phase_mode	= HW_SPI_PHA_MODE_0,
		.mint_mode	= HW_SPI_MINT_DISABLE,
		.xtal_freq	= HW_SPI_FREQ_DIV_2,
		.fifo_mode	= HW_SPI_FIFO_RX_TX,
	};

	hal_pin_rst(2);
	hw_gpio_set_pin_function(HW_LORA_SPI_CLK_PORT, HW_LORA_SPI_CLK_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_LORA_GPIO_FUNC_SPI_CLK);
	hw_gpio_set_pin_function(HW_LORA_SPI_DI_PORT,  HW_LORA_SPI_DI_PIN,
	    HW_GPIO_MODE_INPUT,  HW_LORA_GPIO_FUNC_SPI_DI);
	hw_gpio_set_pin_function(HW_LORA_SPI_DO_PORT,  HW_LORA_SPI_DO_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_LORA_GPIO_FUNC_SPI_DO);
	hw_gpio_configure_pin(   HW_LORA_SPI_CS_PORT,  HW_LORA_SPI_CS_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true);
	hw_gpio_set_pin_function(HW_LORA_RX_PORT,      HW_LORA_RX_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(HW_LORA_TX_PORT,      HW_LORA_TX_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(HW_LORA_DIO0_PORT,    HW_LORA_DIO0_PIN,
	    HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(HW_LORA_DIO1_PORT,    HW_LORA_DIO1_PIN,
	    HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(HW_LORA_DIO2_PORT,    HW_LORA_DIO2_PIN,
	    HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(HW_USER_BTN_PORT,     HW_USER_BTN_PIN,
	    HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO);
	hw_spi_init(HW_LORA_SPI, &cfg);
}

void
hal_periph_init()
{
	power_init();
	cons_reinit();
	hal_lora_init();
	sensor_init();
}

void
hal_init()
{
	wdog_id = sys_watchdog_register(false);
	sys_watchdog_notify(wdog_id);
	wkup_init();
	lora_queue = xQueueCreate(4, sizeof(struct event));
}

void
hal_pin_rst(u1_t val)
{
	if (val == 2) {
		hw_gpio_set_pin_function(HW_LORA_REST_PORT, HW_LORA_REST_PIN,
		    HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO);
	} else {
		hw_gpio_configure_pin(HW_LORA_REST_PORT, HW_LORA_REST_PIN,
		    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, val);
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
	write(1, "hal failed\r\n", 12);
	hw_cpm_reboot_system();
}

void
hal_handle_event(struct event ev)
{
	switch (ev.ev) {
	case EV_LORA_DIO:
		radio_irq_handler(ev.data);
		break;
	case EV_BTN_PRESS:
		button_press(ev.data);
		break;
	case EV_CONS_RX:
		cons_rx();
		break;
	default:
		hal_failed();
	}
}

#define TIMER_PRECISION	((s4_t)(configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ))
#define MIN_SLEEP	(2 * TIMER_PRECISION)
#define MAX_WDOG_SLEEP	sec2osticks(2)

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
	waituntil = hal_ticks() + MAX_WDOG_SLEEP;
}

void
hal_sleep()
{
	s4_t	dt = waituntil - hal_ticks();

	if (dt > MIN_SLEEP) {
		BaseType_t	ret;
		struct event	ev;

		if (dt >= MAX_WDOG_SLEEP) {
#ifdef WATCHDOG_ALWAYS_ON
			dt = MAX_WDOG_SLEEP;
#else
			sys_watchdog_suspend(wdog_id);
#endif
		}
		// Timer precision is 64 ticks.  Sleep for 64 to
		// 128 ticks less than specified.
		// Wait for timer or WKUP_GPIO interrupt
		ret = xQueueReceive(lora_queue, &ev, dt / TIMER_PRECISION - 1);
		sys_watchdog_notify_and_resume(wdog_id);
		if (ret)
			hal_handle_event(ev);
	} else {
		// Busy-wait
		hal_waitUntil(waituntil - 5);
	}
}

void
hal_waitUntil(u4_t time)
{
	struct event	ev;

	while ((s4_t)(time - hal_ticks()) > 0) {
		if (xQueueReceive(lora_queue, &ev, 0)) {
			hal_handle_event(ev);
			break;
		}
	}
}
