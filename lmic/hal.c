#include "oslmic.h"
#include "hal.h"
#include "ad_lmic.h"

#include <hw_cpm.h>
#include <hw_timer1.h>
#include <hw_watchdog.h>
#include <stdio.h> //XXX

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
	__disable_irq();
	hw_watchdog_unfreeze();
	hw_watchdog_gen_RST();
	hw_watchdog_set_pos_val(1);
	hw_watchdog_unfreeze();
	for (;;)
		__WFI();
}

#define LONGSLEEP (2 * (s4_t)(configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ))

static int	needsleep;
static u4_t	waituntil;

u1_t
hal_checkTimer(u4_t targettime)
{
	s4_t	dt;

	dt = targettime - hal_ticks();
	if (dt < 5) {
		// Expiration time is nigh
		printf("one %ld\r\n", dt);
		return 1;
	} else if (dt > LONGSLEEP) {
		// Timer precision is 64 ticks.  Sleep for 64 to
		// 128 ticks less than specified.
		needsleep = 1;
		ad_lmic_set_timer(
		    dt / (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) - 1);
	} else {
		// Don't sleep, busy-wait instead.
		needsleep = 0;
		waituntil = targettime;
	}
	return 0;
}

void
hal_sleep()
{
	if (needsleep) {
		// Wait for timer or WKUP_GPIO interrupt
		ad_lmic_sleep();
	} else {
		// Busy-wait
		hal_waitUntil(waituntil - 5);
	}
}

void
hal_waitUntil(u4_t time)
{
	s4_t	dt = time - hal_ticks();

	if ((s4_t)dt > 0) {
		hal_disableIRQs();
		hw_cpm_delay_usec(osticks2us(dt));
		hal_enableIRQs();
	}
}
