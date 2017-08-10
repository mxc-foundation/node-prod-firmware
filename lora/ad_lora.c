#include <stdbool.h>
#include <hw_wkup.h>
#include <sys_power_mgr.h>

#include "hw/power.h"
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

	if (suspends_active) {
		now = hal_ticks();
		taskENTER_CRITICAL();
		for (id = 0; id < LORA_SUSPENDS; id++) {
			if ((suspends_active & (1 << id)) &&
			    now - suspended_until[id] > 0) {
				suspends_active &= ~(1 << id);
			}
		}
		taskEXIT_CRITICAL();
	}
	if (!suspends_active)
		power(POWER_LORA, false);
	return !suspends_active;
}

static void
ad_lora_sleep_canceled(void)
{
	power(POWER_LORA, true);
}

static void
ad_lora_wake_up_ind(bool arg)
{
	power(POWER_LORA, true);
}

static const adapter_call_backs_t	ad_lora_call_backs = {
	.ad_prepare_for_sleep		= ad_lora_prepare_for_sleep,
	.ad_sleep_canceled		= ad_lora_sleep_canceled,
	.ad_wake_up_ind			= ad_lora_wake_up_ind,
};

void
ad_lora_suspend_sleep(int id, ostime_t period)
{
	ostime_t	until = hal_ticks() + period;

	taskENTER_CRITICAL();
	if (!(suspends_active & (1 << id)) ||
	    (until - suspended_until[id]) > 0) {
		suspends_active |= (1 << id);
		suspended_until[id] = until;
	}
	taskEXIT_CRITICAL();
}

void
ad_lora_allow_sleep(int id)
{
	taskENTER_CRITICAL();
	suspends_active &= ~(1 << id);
	taskEXIT_CRITICAL();
}

void
ad_lora_init()
{
	pm_register_adapter(&ad_lora_call_backs);
}
