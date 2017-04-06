#include <hw_breath.h>
#include <hw_led.h>
#include <hw_timer0.h>
#include <hw_timer2.h>
#include "led.h"

#define ACTION(led, action)	((action) << (led))

uint8_t	status;

static void
timer0_cb(void)
{
	static bool	on;

	on = !on;
	if (status & ACTION(LED_GREEN, LED_BLINK))
		hw_led_enable_led2(on);
	if (status & ACTION(LED_RED, LED_BLINK))
		hw_led_enable_led3(on);
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
	timer0_config	t0cfg = {
		.clk_src	= HW_TIMER0_CLK_SRC_SLOW,
		.on_reload	= 0x1fff,
	};
	timer2_config	t2cfg = {
		.pwm3_start	= 0,
		.pwm3_end	= 0xffff,
		.pwm4_start	= 0,
		.pwm4_end	= 0xffff,
	};

	hw_timer0_init(&t0cfg);
	hw_timer0_register_int(timer0_cb);
	hw_timer2_init(&t2cfg);
	hw_breath_init(&bcfg);
	led_set(LED_RED, LED_BLINK);
}

void
led_set_status(uint8_t s)
{
	status = s;
}

void
led_set(uint8_t led, uint8_t action)
{
	status = (status & ~ACTION(led, LED_FUNC_MASK)) | ACTION(led, action);
	if (status &
	    (ACTION(LED_GREEN, LED_BLINK) | ACTION(LED_RED, LED_BLINK))) {
		hw_timer0_enable();
		hw_timer2_enable();
	} else {
		hw_timer0_disable();
		hw_timer2_disable();
	}
	if (status &
	    (ACTION(LED_GREEN, LED_BREATH) | ACTION(LED_RED, LED_BREATH))) {
		hw_breath_enable();
	} else {
		hw_breath_disable();
	}
	switch (led) {
	case LED_GREEN:
		if (action) {
			hw_led_set_led2_src(action == LED_BREATH ?
			    HW_LED_SRC2_BREATH : HW_LED_SRC2_PWM3);
			hw_led_enable_led2(true);
		} else {
			hw_led_enable_led2(false);
		}
		break;
	case LED_RED:
		if (action) {
			hw_led_set_led3_src(action == LED_BREATH ?
			    HW_LED_SRC3_BREATH : HW_LED_SRC3_PWM4);
			hw_led_enable_led3(true);
		} else {
			hw_led_enable_led3(false);
		}
		break;
	}
}
