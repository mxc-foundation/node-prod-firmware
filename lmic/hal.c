#include "oslmic.h"
#include "hal.h"

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
	__WFI();
}

static u4_t	target;

u1_t
hal_checkTimer(u4_t targettime)
{
	u4_t	ticks, dt/* , value, dummy */;

	ticks = hal_ticks();
	target = targettime;
	dt = targettime - ticks;
	//printf("ticks %lu, targettime %lu, dt %lu (%ld), period %lu (%ld)\r\n", ticks, targettime, dt, (s4_t)dt, TICK_PERIOD, 2 * (s4_t)TICK_PERIOD + 5);
	if ((s4_t)dt < 2 * (s4_t)(configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) + 5) {
		printf("one %ld\r\n", (s4_t)dt);
		return 1;
	} else {
		if (dt >= 0x10000)
			dt = 0x10000;
		//printf("delay 0x%lx (%lu, %lu)\r\n", dt, dt, dt / (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ));
		//vTaskDelay(dt);
		//vTaskSuspend(dt);
		//printf("ok\r\n");
		//value = (dt + ticks) / (1 + dg_configTim1Prescaler);
		//HW_TIMER1_SET_TRIGGER(value, dummy);
		//(void)dummy;
		return 0;
	}
}

void
hal_sleep()
{
	u4_t	dt;

	dt = target - hal_ticks();
	if (dt >= 0x10000)
		dt = 0x10000;
	vTaskDelay(dt / (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ));
}

void
hal_waitUntil(u4_t time)
{
	while (!hal_checkTimer(time))
		;
}
