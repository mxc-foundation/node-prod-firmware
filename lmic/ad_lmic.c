#include <stdio.h> //XXX
#include <stdbool.h>
#include "ad_lmic.h"
#include "oslmic.h"
#include "hal.h"

#include <hw_wkup.h>
#include <sys_power_mgr.h>

#define AD_LMIC_STATUS_ALLOW_SLEEP	0x01
#define AD_LMIC_STATUS_HAL_SLEEPING	0x02

PRIVILEGED_DATA static uint8_t	ad_status;

static bool
ad_lmic_prepare_for_sleep(void)
{
	bool	allow_sleep = (ad_status &
	    (AD_LMIC_STATUS_ALLOW_SLEEP | AD_LMIC_STATUS_HAL_SLEEPING)) ==
	    (AD_LMIC_STATUS_ALLOW_SLEEP | AD_LMIC_STATUS_HAL_SLEEPING);
	//extern unsigned long	_vStackTop[], _pvHeapStart[];

	//printf("%s [%lu]\r\n", __func__, (unsigned long)_vStackTop - (unsigned long)_pvHeapStart);
	//hw_uart_write_buffer(HW_UART1, __func__, strlen(__func__));
	//hw_uart_write_buffer(HW_UART1, "\r\n", 2);
	printf("%s: %s\r\n", __func__, allow_sleep ? "true" : "false");
	if (allow_sleep) {
		hal_suspendWatchdog();
		return true;
	} else {
		return false;
	}
}

static void
ad_lmic_sleep_canceled(void)
{
	printf("%s\r\n", __func__);
}

static void
ad_lmic_wake_up_ind(bool arg)
{
	printf("%s(%d)\r\n", __func__, arg);
}

static void
ad_lmic_xtal16m_ready_ind(void)
{
	printf("%s\r\n", __func__);
}

static const adapter_call_backs_t	ad_lmic_call_backs = {
	.ad_prepare_for_sleep		= ad_lmic_prepare_for_sleep,
	.ad_sleep_canceled		= ad_lmic_sleep_canceled,
	.ad_wake_up_ind			= ad_lmic_wake_up_ind,
	.ad_xtal16m_ready_ind		= ad_lmic_xtal16m_ready_ind,
	.ad_sleep_preparation_time	= 0,
};

void
ad_lmic_allow_sleep(bool allow)
{
	if (allow)
		ad_status |=  AD_LMIC_STATUS_ALLOW_SLEEP;
	else
		ad_status &= ~AD_LMIC_STATUS_ALLOW_SLEEP;
}

void
ad_lmic_hal_sleeping(bool sleeping)
{
	if (sleeping)
		ad_status |=  AD_LMIC_STATUS_HAL_SLEEPING;
	else
		ad_status &= ~AD_LMIC_STATUS_HAL_SLEEPING;
}

void
ad_lmic_init()
{
	pm_register_adapter(&ad_lmic_call_backs);
}
