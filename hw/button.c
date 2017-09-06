#include "lmic/lmic.h"
#include "lora/lora.h"
#include "lora/upgrade.h"

#include "hw/button.h"
#include "hw/hw.h"

#define LONG_PRESS_TIME	sec2osticks(10)

PRIVILEGED_DATA static ostime_t	press_time;

static void
button_cb(osjob_t *job)
{
	ostime_t	now = os_getTime();

	if (now - press_time >= LONG_PRESS_TIME)
		upgrade_reboot(UPGRADE_DEFAULT);
	else if (hw_gpio_get_pin_status(HW_USER_BTN_PORT, HW_USER_BTN_PIN))
		lora_send();
	else
		os_setTimedCallback(job, now + ms2osticks(20), button_cb);
}

void
button_press(ostime_t t)
{
	PRIVILEGED_DATA static osjob_t	button_job;

	press_time = t;
	button_cb(&button_job);
}
