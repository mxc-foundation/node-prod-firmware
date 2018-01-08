/* PCA6416A I/O expander */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <FreeRTOS.h>

#include "hw/hw.h"
#include "hw/i2c.h"
#include "hw/iox.h"

#ifdef HW_IOX_I2C_ADDR

#define IOX_REG_INPUT	0
#define IOX_REG_OUTPUT	2
#define IOX_REG_POL_INV	4
#define IOX_REG_CONF	6

static INITIALISED_PRIVILEGED_DATA uint8_t	xconf[2] = { 0xff, 0xff };
static PRIVILEGED_DATA uint8_t			xoutput[2];

static int
iox_setbit(uint8_t reg, uint8_t *loc, uint8_t bit, bool val)
{
	if (bit & 0x07) {
		loc++;
		reg++;
	}
	bit &= 0x07;
	if (val)
		*loc |= 1 << bit;
	else
		*loc &= ~(1 << bit);
	if (i2c_write(HW_IOX_I2C_ADDR, reg, loc, 1) == -1)
		return -1;
	return 0;
}

int
iox_conf(uint8_t pin, bool input)
{
	return iox_setbit(IOX_REG_CONF, xconf, pin, input);
}

int
iox_set(uint8_t pin, bool val)
{
	return iox_setbit(IOX_REG_OUTPUT, xoutput, pin, val);
}

int
iox_get(uint8_t pin)
{
	uint8_t	port, b;

	port = pin >> 3;
	pin &= 0x07;
	if (i2c_read(HW_IOX_I2C_ADDR, IOX_REG_INPUT + port, &b, 1) == -1)
		return -1;
	return (b >> pin) & 0x01;
}

int
iox_setconf(int conf)
{
	xconf[0] = conf;
	xconf[1] = conf >> 8;
	if (i2c_write(HW_IOX_I2C_ADDR, IOX_REG_CONF, xconf, sizeof(xconf))
	    == -1) {
		return -1;
	}
	return 0;
}

int
iox_getconf()
{
	return (int)xconf[1] << 8 | xconf[0];
}

int
iox_setpins(int pins)
{
	xoutput[0] = pins;
	xoutput[1] = pins >> 8;
	if (i2c_write(HW_IOX_I2C_ADDR, IOX_REG_OUTPUT, xoutput, sizeof(xoutput))
	    == -1) {
		return -1;
	}
	return 0;
}

int
iox_getpins()
{
	uint8_t	buf[2];

	if (i2c_read(HW_IOX_I2C_ADDR, IOX_REG_INPUT, buf, sizeof(buf)) == -1)
		return -1;
	return (int)buf[1] << 8 | buf[0];
}

#endif /* HW_IOX_I2C_ADDR */
