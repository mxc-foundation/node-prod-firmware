#include <stdbool.h>
#include <hw_wkup.h>
#include <sys_power_mgr.h>

#include "lmic/oslmic.h"
#include "lmic/hal.h"
#include "lora/ad_lora.h"

PRIVILEGED_DATA static uint8_t	suspends_active;
PRIVILEGED_DATA static ostime_t	suspended_until[LORA_SUSPENDS];

static bool
ad_lora_prepare_for_sleep(void)
{
	ostime_t	now;
	int		id;

	if (!suspends_active)
		return true;
	now = hal_ticks();
	for (id = 0; id < LORA_SUSPENDS; id++) {
		if ((suspends_active & (1 << id)) &&
		    now - suspended_until[id] > 0) {
			suspends_active &= ~(1 << id);
		}
	}
	return !suspends_active;
}

static const adapter_call_backs_t	ad_lora_call_backs = {
	.ad_prepare_for_sleep		= ad_lora_prepare_for_sleep,
};

void
ad_lora_suspend_sleep(int id, ostime_t period)
{
	ostime_t	until = hal_ticks() + period;

	if (!(suspends_active & (1 << id)) ||
	    (until - suspended_until[id]) > 0) {
		suspends_active |= (1 << id);
		suspended_until[id] = until;
	}
}

void
ad_lora_allow_sleep(int id)
{
	suspends_active &= ~(1 << id);
}

void
ad_lora_init()
{
	pm_register_adapter(&ad_lora_call_backs);
}
