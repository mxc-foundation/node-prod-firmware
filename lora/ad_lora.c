#include <stdbool.h>
#include <hw_wkup.h>
#include <sys_power_mgr.h>

#include "lmic/oslmic.h"
#include "lmic/hal.h"
#include "lora/ad_lora.h"

PRIVILEGED_DATA static ostime_t	suspended_until;

static bool
ad_lora_prepare_for_sleep(void)
{
	return (int32_t)(hal_ticks() - suspended_until) > 0;
}

static const adapter_call_backs_t	ad_lora_call_backs = {
	.ad_prepare_for_sleep		= ad_lora_prepare_for_sleep,
};

void
ad_lora_suspend_sleep(ostime_t period)
{
	ostime_t	until = hal_ticks() + period;

	if ((int32_t)(until - suspended_until) > 0)
		suspended_until = until;
}

void
ad_lora_allow_sleep(void)
{
	suspended_until = hal_ticks();
}

void
ad_lora_init()
{
	pm_register_adapter(&ad_lora_call_backs);
}
