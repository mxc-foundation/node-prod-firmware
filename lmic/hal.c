#include "oslmic.h"
#include "hal.h"

#include <hw_cpm.h>

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

void
hal_waitUntil(u4_t time)
{
	//while ((s4_t)(hal_ticks() - time) < 0)
	while (!hal_checkTimer(time))
		;
}
