/* LoRa MatchX protocol */

#include <stdint.h>
#include "hw/led.h"
#include "ble.h"
#include "lmic/lmic.h"
#include "lora/param.h"
#include "lora/util.h"

/* CMD_REBOOT_UPGRADE */
#define UPGRADE_BIT		0x80
#define REBOOT_TIMEOUT_MASK	0x07

static const ostime_t	reboot_timeouts[] = {
	sec2osticks(2),
	sec2osticks(5 * 60),	/* UPGRADE_DEFAULT */
	sec2osticks(15 * 60),
	sec2osticks(30 * 60),
	sec2osticks(60 * 60),
	sec2osticks(2 * 60 * 60),
	sec2osticks(4 * 60 * 60),
	/* TBD */
};

static void
do_reboot(osjob_t *job)
{
	PRIVILEGED_DATA static uint8_t	cnt;

	/* If SUOTA is active, delay reboot, but force it after
	 * 256 * 5s (21 min 20 sec). */
	if (ble_is_suota_ongoing() && --cnt) {
		os_setTimedCallback(job, hal_ticks() + sec2osticks(5),
		    do_reboot);
		return;
	}
	hw_cpm_reboot_system();
}

static void
schedule_reboot(uint8_t flags)
{
	PRIVILEGED_DATA static osjob_t	reboot_job;
	uint8_t				tmout;

	tmout = flags & REBOOT_TIMEOUT_MASK;
	if (tmout >= ARRAY_SIZE(reboot_timeouts))
		return; // XXX
	os_setTimedCallback(&reboot_job,
	    hal_ticks() + reboot_timeouts[tmout],
	    do_reboot);
}

void
upgrade_reboot(uint8_t flags)
{
	if ((flags & REBOOT_TIMEOUT_MASK) >= ARRAY_SIZE(reboot_timeouts))
		return; // XXX
	if (flags & UPGRADE_BIT) {
		param_set(PARAM_SUOTA, &flags, sizeof(flags));
		led_notify(LED_STATE_REBOOTING);
		schedule_reboot(0);
	} else {
		schedule_reboot(flags);
	}
}

void
upgrade_init()
{
	uint8_t	flags;

	if (param_get(PARAM_SUOTA, &flags, sizeof(flags)) == 0 || flags == 0)
		return;
	/*
	 * Device will be ready for upgrade if suota parameter is set.
	 * Last 3 bits of the parameter define the timeout of reset if
	 * suota upgrade doesn't start in reboot_timeouts.
	 * Example: If the suota parameter is 0x81, BLE will be open for
	 * 5 minutes and then will reboot if suota upgrade doesn't start.
	 */
	if (flags & UPGRADE_BIT) {
		ble_on();
		schedule_reboot(flags);
	}
	flags = 0;
	param_set(PARAM_SUOTA, &flags, sizeof(flags));
}
