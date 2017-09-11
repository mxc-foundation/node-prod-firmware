#include <sys/types.h>
#include <FreeRTOS.h>
#include <hw_gpio.h>
#include "hw/hw.h"
#include "lmic/oslmic.h"
#include "lora/param.h"
#include "lora/util.h"
#include "gps.h"
#include "sensor.h"
#include "temp.h"

#define SENSOR_TYPE_UNKNOWN	0
#define SENSOR_TYPE_GPS		1
#define SENSOR_TYPE_TEMP	2
static uint8_t	sensor_type;

static const ostime_t	sensor_periods[] = {
	sec2osticks(60),	/* default */
	sec2osticks(10),
	sec2osticks(30),
	sec2osticks(60),
	sec2osticks(2 * 60),
	sec2osticks(5 * 60),
	sec2osticks(10 * 60),
	sec2osticks(30 * 60),
	sec2osticks(60 * 60),
	sec2osticks(2 * 60 * 60),
	sec2osticks(5 * 60 * 60),
	sec2osticks(12 * 60 * 60),
};

struct sensor_callbacks {
	void		(*init)(void);
	void		(*prepare)(void);
	ostime_t	(*data_ready)(void);
	int		(*read)(char *, int);
	void		(*txstart)(void);
};

const struct sensor_callbacks	sensor_cb[] = {
	[SENSOR_TYPE_UNKNOWN]	= {
	},
	[SENSOR_TYPE_GPS]	= {
		.init		= gps_init,
		.prepare	= gps_prepare,
		.data_ready	= gps_data_ready,
		.read		= gps_read,
		.txstart	= gps_txstart,
	},
	[SENSOR_TYPE_TEMP]	= {
		.read		= temp_read,
	},
};

static inline void
detect_sensor(void)
{
	sensor_type = SENSOR_TYPE_GPS;
}

void
sensor_init()
{
	detect_sensor();
	if (sensor_cb[sensor_type].init)
		sensor_cb[sensor_type].init();
}

void
sensor_prepare()
{
	if (sensor_cb[sensor_type].prepare)
		sensor_cb[sensor_type].prepare();
}

ostime_t
sensor_data_ready()
{
	if (sensor_cb[sensor_type].data_ready)
		return sensor_cb[sensor_type].data_ready();
	else
		return 0;
}

size_t
sensor_get_data(char *buf, int len)
{
	int	rlen;

	if (len <= 0)
		return 0;
	buf[0] = sensor_type;
	rlen = 1;
	if (sensor_cb[sensor_type].read)
		rlen += sensor_cb[sensor_type].read(buf + 1, len - 1);
	return rlen;
}

ostime_t
sensor_period(void)
{
	uint8_t	idx = 0;

	if (param_get(PARAM_SENSOR_PERIOD, &idx, sizeof(idx)) &&
	    idx >= ARRAY_SIZE(sensor_periods)) {
		idx = 0;
	}
	return sensor_periods[idx];
}

void
sensor_txstart(void)
{
	if (sensor_cb[sensor_type].txstart)
		return sensor_cb[sensor_type].txstart();
}
