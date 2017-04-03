#include <hw_breath.h>
#include <hw_led.h>
#include <hw_timer0.h>
#include <hw_timer2.h>
#include "led.h"

uint8_t	status;

static void
timer0_cb(void)
{
	static bool	on;

	on = !on;
	switch (status) {
	case LED_BLINK_RED:
		hw_led_enable_led2(false);
		hw_led_enable_led3(on);
		break;
	case LED_BLINK_GREEN:
		hw_led_enable_led2(on);
		hw_led_enable_led3(false);
		break;
	}
}

void
led_init(void)
{
	breath_config	bcfg = {
		.dc_min		= 0,
		.dc_max		= 0xff,
		.dc_step	= 0x20,
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
	hw_timer0_enable();
	hw_timer2_init(&t2cfg);
	hw_timer2_enable();
	hw_breath_init(&bcfg);
	hw_led_set_led2_src(HW_LED_SRC2_PWM3);
	hw_led_enable_led3(true);
	hw_led_set_led3_src(HW_LED_SRC3_PWM4);
	hw_led_enable_led3(true);
	hw_breath_enable();
}

void
led_set_status(uint8_t s)
{
	status = s;
}
