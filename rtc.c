#include <sdk_defs.h>
#include <hw_timer1.h>
#include "rtc.h"

static uint32_t	rtc_time, rtc_previous_time;
uint32_t	lastfinetime, intcnt; // XXX

void
rtc_init()
{
	NVIC_SetPriority(SWTIM1_IRQn, 3);
	hw_timer1_lp_clk_init();
	hw_timer1_int_enable();
	hw_timer1_enable();
}

uint32_t
rtc_get_fromISR()
{
	uint32_t	prescaled, fine;

        HW_TIMER1_GET_INSTANT(prescaled, fine);
        rtc_time += (fine - rtc_previous_time) & LP_CNT_NATIVE_MASK;
        rtc_previous_time = fine;
	return rtc_time;
}

uint32_t
rtc_get()
{
	uint32_t	prescaled, fine;

	GLOBAL_INT_DISABLE();
        HW_TIMER1_GET_INSTANT(prescaled, fine);
        rtc_time += (fine - rtc_previous_time) & LP_CNT_NATIVE_MASK;
        rtc_previous_time = fine;
	GLOBAL_INT_RESTORE();
	return rtc_time;
}

__RETAINED_CODE void
SWTIM1_Handler(void)
{
	uint32_t	prescaled, fine;

        HW_TIMER1_GET_INSTANT(prescaled, fine);
        rtc_time += (fine - rtc_previous_time) & LP_CNT_NATIVE_MASK;
        rtc_previous_time = fine;
	lastfinetime = fine;
	intcnt++;
	//NVIC_ClearPendingIRQ(SWTIM1_IRQn);
}
