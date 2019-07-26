#include <FreeRTOS.h>
#include <hw_breath.h>
#include <hw_led.h>
#include <hw_timer0.h>
#include <hw_timer2.h>
#include <hw_usb_charger.h>
#include <sys_charger.h>

#include "lmic/oslmic.h"
#include "lora/ad_lora.h"
#include "lora/util.h"
#include "hw.h"
#include "led.h"

PRIVILEGED_DATA static uint8_t	sys_status;

#define LED_BATTERY_OK		0x00
#define LED_BATTERY_LOW		0x01
#define LED_BATTERY_CHARGING	0x02
#define LED_BATTERY_CHARGED	0x03

PRIVILEGED_DATA static uint8_t	battery_status;

#define LED_FUNC_MASK		0x07
#define LED_OFF			0x00
#define LED_BREATH		0x01
#define LED_BLINK_NORMAL	0x02
#define LED_BLINK_RARE		0x03
#define LED_BLINK_FAST		0x04
#define LED_BLINK_ALTERNATE	0x05

#define LED_COLOUR_MASK		0x38
#define LED_RED			0x08
#define LED_GREEN		0x10
#define LED_BLUE		0x20
#define LED_YELLOW		(LED_RED | LED_GREEN)

PRIVILEGED_DATA static uint8_t	led_status;

#define NORMAL_BLINK_PERIOD	ms2osticks(250)
#define FAST_BLINK_PERIOD	ms2osticks(50)
#define RARE_BLINK_ON_PERIOD	ms2osticks(50)
#define RARE_BLINK_OFF_PERIOD	sec2osticks(4)
#define UPDATE_INTERVAL		sec2osticks(10)

static const uint8_t	led_sys_stati[] = {
	[LED_STATE_IDLE]		= LED_OFF,
#ifdef FEATURE_LED_RGB
	[LED_STATE_BOOTING]		= LED_BLUE   | LED_BLINK_NORMAL,
#else
	[LED_STATE_BOOTING]		= LED_YELLOW | LED_BLINK_NORMAL,
#endif
	[LED_STATE_JOINING]		= LED_RED    | LED_BLINK_NORMAL,
	[LED_STATE_SAMPLING_SENSOR]	= LED_YELLOW | LED_BLINK_ALTERNATE,
	[LED_STATE_SENDING]		= LED_GREEN  | LED_BLINK_NORMAL,
	[LED_STATE_REBOOTING]		= LED_RED    | LED_BLINK_FAST,
};

static const uint8_t	led_battery_stati[] = {
	[LED_BATTERY_OK]		= LED_OFF,
	[LED_BATTERY_LOW]		= LED_RED   | LED_BLINK_RARE,
	[LED_BATTERY_CHARGING]		= LED_RED   | LED_BREATH,
	[LED_BATTERY_CHARGED]		= LED_GREEN | LED_BREATH,
};

#if defined FEATURE_LED_RGB

#if defined FEATURE_LED_RGB_REV

#define LED_SET_RED_BREATH()	hw_led_set_led3_src(HW_LED_SRC3_BREATH)
#define LED_SET_RED_PWM()	hw_led_set_led3_src(HW_LED_SRC3_PWM4)
#define LED_SET_GREEN_BREATH()	hw_led_set_led2_src(HW_LED_SRC2_BREATH)
#define LED_SET_GREEN_PWM()	hw_led_set_led2_src(HW_LED_SRC2_PWM3)
#define LED_SET_BLUE_BREATH()	hw_led_set_led1_src(HW_LED_SRC1_BREATH)
#define LED_SET_BLUE_PWM()	hw_led_set_led1_src(HW_LED_SRC1_PWM2)
#define LED_ENABLE_RED(state)	hw_led_enable_led3(state)
#define LED_ENABLE_GREEN(state)	hw_led_enable_led2(state)
#define LED_ENABLE_BLUE(state)	hw_led_enable_led1(state)

#else /* !FEATURE_LED_RGB_REV */

#define LED_SET_RED_BREATH()	hw_led_set_led1_src(HW_LED_SRC1_BREATH)
#define LED_SET_RED_PWM()	hw_led_set_led1_src(HW_LED_SRC1_PWM2)
#define LED_SET_GREEN_BREATH()	hw_led_set_led2_src(HW_LED_SRC2_BREATH)
#define LED_SET_GREEN_PWM()	hw_led_set_led2_src(HW_LED_SRC2_PWM3)
#define LED_SET_BLUE_BREATH()	hw_led_set_led3_src(HW_LED_SRC3_BREATH)
#define LED_SET_BLUE_PWM()	hw_led_set_led3_src(HW_LED_SRC3_PWM4)
#define LED_ENABLE_RED(state)	hw_led_enable_led1(state)
#define LED_ENABLE_GREEN(state)	hw_led_enable_led2(state)
#define LED_ENABLE_BLUE(state)	hw_led_enable_led3(state)

#endif /* FEATURE_LED_RGB_REV */

#elif defined FEATURE_LED_RG

#define LED_SET_RED_BREATH()	hw_led_set_led3_src(HW_LED_SRC3_BREATH)
#define LED_SET_RED_PWM()	hw_led_set_led3_src(HW_LED_SRC3_PWM4)
#define LED_SET_GREEN_BREATH()	hw_led_set_led2_src(HW_LED_SRC2_BREATH)
#define LED_SET_GREEN_PWM()	hw_led_set_led2_src(HW_LED_SRC2_PWM3)
#define LED_SET_BLUE_BREATH()
#define LED_SET_BLUE_PWM()
#define LED_ENABLE_RED(state)	hw_led_enable_led3(state)
#define LED_ENABLE_GREEN(state)	hw_led_enable_led2(state)
#define LED_ENABLE_BLUE(state)

#else
#error "Unknown LED setting"
#endif

static void
led_conf_timers()
{
	if ((led_status & LED_FUNC_MASK) == LED_BREATH) {
		hw_breath_enable();
		LED_SET_RED_BREATH();
		LED_SET_GREEN_BREATH();
		LED_SET_BLUE_BREATH();
	} else {
		hw_breath_disable();
		LED_SET_RED_PWM();
		LED_SET_GREEN_PWM();
		LED_SET_BLUE_PWM();
	}

	if ((led_status & LED_FUNC_MASK) >= LED_BLINK_NORMAL)
		hw_timer2_enable();
	else
		hw_timer2_disable();
}

static bool
led_update_status()
{
	uint8_t	s = LED_OFF;

	if (sys_status != LED_STATE_IDLE) {
		if (sys_status < ARRAY_SIZE(led_sys_stati))
			s = led_sys_stati[sys_status];
	} else {
		if (battery_status < ARRAY_SIZE(led_battery_stati))
			s = led_battery_stati[battery_status];
	}
	if (s == led_status)
		return false;
	led_status = s;

	led_conf_timers();
	return true;
}

static bool
led_update_battery(void)
{
	uint8_t	s = LED_BATTERY_OK;

#ifdef FEATURE_BATTERY
	if (hw_charger_is_charging())
		s = LED_BATTERY_CHARGING;
	else if (hw_charger_check_vbus())
		s = LED_BATTERY_CHARGED;
	else if (usb_charger_is_battery_low())
		s = LED_BATTERY_LOW;
	if (s == battery_status)
		return false;
#endif
	battery_status = s;
	return led_update_status();
}

static void
led_cb(osjob_t *job)
{
	PRIVILEGED_DATA static osjob_t	led_job;
	PRIVILEGED_DATA static bool	on;
	ostime_t			delay = UPDATE_INTERVAL;
	bool				red_inverted = false;
	bool				updated;

	updated = led_update_battery() || !job;
	switch (led_status & LED_FUNC_MASK) {
	case LED_OFF:
		on = false;
		break;
	case LED_BREATH:
		on = true;
		break;
	default:
		on = updated || !on;
		switch (led_status & LED_FUNC_MASK) {
		case LED_BLINK_ALTERNATE:
			red_inverted = true;
			/* NO BREAK FALLTHROUGH */
		case LED_BLINK_NORMAL:
			delay = NORMAL_BLINK_PERIOD;
			break;
		case LED_BLINK_FAST:
			delay = FAST_BLINK_PERIOD;
			break;
		default:
			delay = on ? RARE_BLINK_ON_PERIOD :
			    RARE_BLINK_OFF_PERIOD;
			break;
		}
		break;
	}
	LED_ENABLE_RED((on ^ red_inverted) && !!(led_status & LED_RED));
	LED_ENABLE_GREEN(on && !!(led_status & LED_GREEN));
	LED_ENABLE_BLUE(on && !!(led_status & LED_BLUE));
	os_setTimedCallback(&led_job, hal_ticks() + delay, led_cb);
	if (on || red_inverted)
		ad_lora_suspend_sleep(LORA_SUSPEND_LED, delay);
	else
		ad_lora_allow_sleep(LORA_SUSPEND_LED);
}

void
led_notify(uint8_t s)
{
	if (sys_status == s)
		return;
	sys_status = s;
	if (led_update_status())
		led_cb(0);
}

void
led_init(void)
{
	breath_config	bcfg = {
		.dc_min		= 0,
		.dc_max		= 0xff,
		.dc_step	= 0xff,
		.freq_div	= 0xff,
		.polarity	= HW_BREATH_PWM_POL_POS,
	};
	timer2_config	t2cfg = {
		.frequency	= HW_TIMER2_MAX_VALUE,
#ifdef FEATURE_LED_RGB
		.pwm2_start	= 0,
		.pwm2_end	= 0xffff,
#endif
		.pwm3_start	= 0,
		.pwm3_end	= 0xffff,
		.pwm4_start	= 0,
		.pwm4_end	= 0xffff,
	};

	hw_timer2_init(&t2cfg);
	hw_breath_init(&bcfg);
	led_conf_timers();
}
