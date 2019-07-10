#include <hw_wkup.h>

#include "lmic/lmic.h"
#include "lora/lora.h"
#include "lora/upgrade.h"
#include "hw/button.h"
#include "hw/hw.h"

#ifdef FEATURE_USER_BUTTON

#define LONG_PRESS_TIME	sec2osticks(10)

#if HW_USER_BTN_ACTIVE == HW_WKUP_PIN_STATE_HIGH
#define USER_BTN_DOWN(port, pin)	hw_gpio_get_pin_status(port, pin)
#else
#define USER_BTN_DOWN(port, pin)	!hw_gpio_get_pin_status(port, pin)
#endif

PRIVILEGED_DATA static ostime_t	press_time;

static void
button_cb(osjob_t *job)
{
	ostime_t	now = os_getTime();

	if (now - press_time >= LONG_PRESS_TIME)
	{// If the button is pressed long enough start upgrade.
		upgrade_reboot(UPGRADE_DEFAULT);
	}else if (!USER_BTN_DOWN(HW_USER_BTN_PORT, HW_USER_BTN_PIN))
	{// If the button is released already send lora package.
	  lora_send();
	}
	else
	{// If the button is still pressed check until released.
	  os_setTimedCallback(job, now + ms2osticks(20), button_cb);
	}
}

void
button_press(ostime_t t)
{
	PRIVILEGED_DATA static osjob_t	button_job;

	// Save the time when the button is pressed to check duration.
	press_time = t;
	button_cb(&button_job);
}

#endif /* FEATURE_USER_BUTTON */
