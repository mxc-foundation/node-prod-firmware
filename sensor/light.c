#include <stdint.h>
#include <limits.h>

#include "lmic/oslmic.h"
#include "lora/util.h"
#include "hw/hw.h"
#include "hw/i2c.h"
#include "light.h"

#ifdef FEATURE_SENSOR_LIGHT

/*
 * This sensor is the Grove - Digital Light Sensor
 * which has TSL2561. The communication is with I2C.
 * */

#define SZ	3

#define REG_CONTROL	0x80
#define REG_TIMING	0x81
#define REG_INTERRUPT	0x86
#define REG_ID		0x8a
#define REG_DATA0LOW	0x8c
#define REG_DATA0HIGH	0x8d
#define REG_DATA1LOW	0x8e
#define REG_DATA1HIGH	0x8f

#define LUX_SCALE	14	// scale by 2^14
#define RATIO_SCALE	9	// scale ratio by 2^9

//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// Integration time scaling factors
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−

#define CH_SCALE	10	// scale channel values by 2^10
#define CHSCALE_TINT0	0x7517	// 322/11 * 2^CH_SCALE
#define CHSCALE_TINT1	0x0fe7	// 322/81 * 2^CH_SCALE

//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// T, FN, and CL Package coefficients
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// For Ch1/Ch0=0.00 to 0.50
//           Lux/Ch0=0.0304−0.062*((Ch1/Ch0)^1.4)
//           piecewise approximation
//                  For Ch1/Ch0=0.00 to 0.125:
//                         Lux/Ch0=0.0304−0.0272*(Ch1/Ch0)
//
//                  For Ch1/Ch0=0.125 to 0.250:
//                         Lux/Ch0=0.0325−0.0440*(Ch1/Ch0)
//
//                  For Ch1/Ch0=0.250 to 0.375:
//                         Lux/Ch0=0.0351−0.0544*(Ch1/Ch0)
//
//                  For Ch1/Ch0=0.375 to 0.50:
//                         Lux/Ch0=0.0381−0.0624*(Ch1/Ch0)
//
// For Ch1/Ch0=0.50 to 0.61:
//           Lux/Ch0=0.0224−0.031*(Ch1/Ch0)
//
// For Ch1/Ch0=0.61 to 0.80:
//           Lux/Ch0=0.0128−0.0153*(Ch1/Ch0)
//
// For Ch1/Ch0=0.80 to 1.30:
//           Lux/Ch0=0.00146−0.00112*(Ch1/Ch0)
//
// For Ch1/Ch0>1.3:
//           Lux/Ch0=0
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
#define K1T	0x0040	// 0.125 * 2^RATIO_SCALE
#define B1T	0x01f2	// 0.0304 * 2^LUX_SCALE
#define M1T	0x01be	// 0.0272 * 2^LUX_SCALE
#define K2T	0x0080	// 0.250 * 2^RATIO_SCALE
#define B2T	0x0214	// 0.0325 * 2^LUX_SCALE
#define M2T	0x02d1	// 0.0440 * 2^LUX_SCALE
#define K3T	0x00c0	// 0.375 * 2^RATIO_SCALE
#define B3T	0x023f	// 0.0351 * 2^LUX_SCALE
#define M3T	0x037b	// 0.0544 * 2^LUX_SCALE
#define K4T	0x0100	// 0.50 * 2^RATIO_SCALE
#define B4T	0x0270	// 0.0381 * 2^LUX_SCALE
#define M4T	0x03fe	// 0.0624 * 2^LUX_SCALE
#define K5T	0x0138	// 0.61 * 2^RATIO_SCALE
#define B5T	0x016f	// 0.0224 * 2^LUX_SCALE
#define M5T	0x01fc	// 0.0310 * 2^LUX_SCALE
#define K6T	0x019a	// 0.80 * 2^RATIO_SCALE
#define B6T	0x00d2	// 0.0128 * 2^LUX_SCALE
#define M6T	0x00fb	// 0.0153 * 2^LUX_SCALE
#define K7T	0x029a	// 1.3 * 2^RATIO_SCALE
#define B7T	0x0018	// 0.00146 * 2^LUX_SCALE
#define M7T	0x0012	// 0.00112 * 2^LUX_SCALE
#define K8T	0x029a	// 1.3 * 2^RATIO_SCALE
#define B8T	0x0000	// 0.000 * 2^LUX_SCALE
#define M8T	0x0000	// 0.000 * 2^LUX_SCALE

static const struct {
	uint16_t	k, b, m;
} coeff[] = {
	{ K1T, B1T, M1T },
	{ K2T, B2T, M2T },
	{ K3T, B3T, M3T },
	{ K4T, B4T, M4T },
	{ K5T, B5T, M5T },
	{ K6T, B6T, M6T },
	{ K7T, B7T, M7T },
};

static uint32_t
calclux(uint16_t d0, uint16_t d1)
{
	uint32_t	chan0, chan1, ratio, b, m, temp;
	int		i;

	chan0 = (d0 * (CHSCALE_TINT0 << 4)) >> CH_SCALE;
	chan1 = (d1 * (CHSCALE_TINT0 << 4)) >> CH_SCALE;
	ratio = 0;
	if (chan0 != 0)
		ratio = ((chan1 << (RATIO_SCALE + 1)) / chan0 + 1) >> 1;
	b = 0;
	m = 0;
	for (i = 0; i < (int)ARRAY_SIZE(coeff); i++) {
		if (ratio <= coeff[i].k) {
			b = coeff[i].b;
			m = coeff[i].m;
			break;
		}
	}
	if ((chan0 * b) >= (chan1 * m))
		temp = (chan0 * b) - (chan1 * m);
	else
		temp = 0;
	return (temp + (1 << (LUX_SCALE - 1))) >> LUX_SCALE;
}

static int
light_write_reg(uint8_t reg, uint8_t val)
{
	return i2c_write(HW_SENSOR_GROVE_LIGHT_I2C_ADDR, reg, &val, 1);
}

static int
light_read_reg(uint8_t reg)
{
	uint8_t	val;

	if (i2c_read(HW_SENSOR_GROVE_LIGHT_I2C_ADDR, reg, &val, 1) == -1)
		return -1;
	return val;
}

static int
light_power_on()
{
	ostime_t	t;
	int		val;

	if (light_write_reg(REG_CONTROL, 0x03) == -1)
		return -1;

	t = os_getTime() + ms2osticks(14);
	while ((s4_t)(t - os_getTime()) > 0)
		;
	if ((val = light_read_reg(REG_CONTROL)) == -1)
		return -1;
	return (val & 0x03) == 0x03 ? 0 : -1;
}

static int
light_power_off()
{
	int	val;

	if (light_write_reg(REG_CONTROL, 0x00) == -1)
		return -1;
	if ((val = light_read_reg(REG_CONTROL)) == -1)
		return -1;
	return (val & 0x03) == 0x00 ? 0 : -1;
}

int
light_read(char *buf, int len)
{
	int		i, val;
	uint32_t	lux;
	uint8_t		tbuf[4];

	if (len < SZ)
		return 0;
	light_power_on();
	for (i = 0; i < 4; i++) {
		if ((val = light_read_reg(REG_DATA0LOW + i)) == -1) {
			light_power_off();
			return 0;
		}
		tbuf[i] = val;
	}
	light_power_off();
	lux = calclux(tbuf[1] << 8 | tbuf[0], tbuf[3] << 8 | tbuf[2]);
	buf[0] = lux;
	buf[1] = lux >> 8;
	buf[2] = lux >> 16;
	return SZ;
}

void
light_init()
{
	ostime_t	t;
	int		val;

	t = os_getTime() + ms2osticks(50);
	while ((val = light_read_reg(REG_ID)) == -1) {
		if ((s4_t)(t - os_getTime()) < 0)
			return;
	}
	if (light_power_on() == -1 ||
	    light_write_reg(REG_TIMING, 0x00) == -1 ||
	    light_write_reg(REG_INTERRUPT, 0x00) == -1 ||
	    light_power_off() == -1) {
		return;
	}
}

#endif /* FEATURE_SENSOR_LIGHT */
