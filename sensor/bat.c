#include <ad_battery.h>
#include "hw/hw.h"
#include "sensor/bat.h"

#ifdef FEATURE_BATTERY

#define mV_DIV	10
#define mV_MIN	2000				/* 2000 mV */
#define mV_MAX	(mV_MIN + 0xff * mV_DIV)	/* 4550 mV */

uint8_t
bat_level()
{
	uint16_t	voltage;
	battery_source	bat;

	bat = ad_battery_open();
	voltage = ad_battery_raw_to_mvolt(bat, ad_battery_read(bat));
	ad_battery_close(bat);
	if (voltage < mV_MIN)
		voltage = mV_MIN;
	else if (voltage > mV_MAX)
		voltage = mV_MAX;
	return (voltage - mV_MIN) / mV_DIV;
}

#endif /* FEATURE_BATTERY */
