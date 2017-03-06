#include <stdio.h>

#include <hw_wkup.h>

#include <FreeRTOS.h>
#include "lmic/lmic.h"
#include "ad_lmic.h"

#define QLENGTH		1
static TimerHandle_t	lora_timer;
static QueueHandle_t	lora_queue;

#define debug //XXX

#ifdef debug
#define SET_HIGH(pin, port)	hw_gpio_set_active(pin, port)
#define SET_LOW(pin, port)	hw_gpio_set_inactive(pin, port)
#else
#define SET_HIGH(pin, port)
#define SET_LOW(pin, port)
#endif

static void
timer_cb(TimerHandle_t timer)
{
	uint8_t	dummy = 0;

	SET_HIGH(4, 2);
	xQueueSend(lora_queue, &dummy, 0);
	SET_LOW(4, 2);
}

static int
handle_dio(uint8_t dio)
{
	BaseType_t	woken = 0;
	uint8_t		dummy = 0;

	radio_irq_handler(dio);

	SET_HIGH(4, 2);
	xQueueSendFromISR(lora_queue, &dummy, &woken);
	SET_LOW(4, 2);
	return woken;
}

static void
wkup_intr_cb(void)
{
	int	woken = 0;

	if (hw_gpio_get_pin_status(HAL_LORA_DIO0_PORT, HAL_LORA_DIO0_PIN))
		woken = handle_dio(0);
	else if (hw_gpio_get_pin_status(HAL_LORA_DIO1_PORT, HAL_LORA_DIO1_PIN))
		woken = handle_dio(1);
#if 0
	else if (hw_gpio_get_pin_status(HAL_LORA_DIO2_PORT, HAL_LORA_DIO2_PIN))
		woken = handle_dio(2);
#endif
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
ad_lmic_init()
{
	lora_queue = xQueueCreate(5, sizeof(uint8_t));
	lora_timer = xTimerCreate("LMICtimer", 1, pdFALSE, NULL, timer_cb);
	wkup_init();
	os_init();
#ifdef debug
	hw_gpio_set_pin_function(1, 0, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(4, 2, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
#endif
}

void
ad_lmic_set_timer(uint32_t ticks)
{
	//SET_HIGH(4, 2);
	xTimerChangePeriod(lora_timer, ticks, 0);
}

void
ad_lmic_sleep()
{
	uint8_t	dummy;

	SET_HIGH(1, 0);
	xQueueReceive(lora_queue, &dummy, portMAX_DELAY);
	SET_LOW(1, 0);
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
