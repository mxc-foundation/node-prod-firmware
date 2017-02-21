#include <unistd.h> // XXX
#include "oslmic.h"
#include "hal.h"
#include "rtc.h"

#include <hw_cpm.h>
#include <hw_timer1.h>
#include <hw_watchdog.h>

#define WDG_RESET()	hw_watchdog_set_pos_val(dg_configWDOG_RESET_VALUE)

void
hal_init()
{
}

void
hal_pin_rst(u1_t val)
{
}

u1_t
hal_spi(u1_t outval)
{
	hw_spi_wait_while_busy(HW_SPI2);
	hw_spi_fifo_write8(HW_SPI2, outval);
	hw_spi_wait_while_busy(HW_SPI2);
	return hw_spi_fifo_read8(HW_SPI2);
}

void
hal_failed()
{
	hw_cpm_reboot_system();
}

u4_t
hal_ticks()
{
#if 0
	uint32_t	prescaled, fine;

	HW_TIMER1_GET_INSTANT(prescaled, fine);
	return fine;
#endif
	return rtc_get();
}

u1_t
hal_checkTimer(u4_t targettime)
{
	u4_t	dt = targettime - hal_ticks();
	u4_t	ticks, value, dummy;

	WDG_RESET();
	ticks = hal_ticks();
	dt = targettime - ticks;
	if ((s4_t)dt < 5) {
		write(1, ",", 1); // XXX
		return 1;
	} else {
		write(1, ".", 1); // XXX
		if (dt >= 0x20000)
			dt = 0x20000;
		value = (dt + ticks) / (1 + dg_configTim1Prescaler);
		HW_TIMER1_SET_TRIGGER(value, dummy);
		(void)dummy;
		return 0;
	}
}

void
hal_waitUntil(u4_t time)
{
	//while ((s4_t)(hal_ticks() - time) < 0)
	while (!hal_checkTimer(time))
		;
}
