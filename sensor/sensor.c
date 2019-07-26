#include <sys/types.h>
#include <FreeRTOS.h>
#include <hw_gpio.h>
#include "hw/hw.h"
#include "lmic/oslmic.h"
#include "lora/param.h"
#include "lora/util.h"
#include "gps.h"
#include "light.h"
#include "sensor.h"
#include "temp.h"

#define DEFAULT_SENSOR_PERIOD	sec2osticks(60)

static const ostime_t	sensor_periods[] = {
	DEFAULT_SENSOR_PERIOD,
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

#ifdef FEATURE_SENSOR

#define SENSOR_TYPE_UNKNOWN	0
#define SENSOR_TYPE_GPS		  1
#define SENSOR_TYPE_TEMP	  2
#define SENSOR_TYPE_LIGHT	  3
PRIVILEGED_DATA static uint8_t	sensor_type[SENSOR_MAX];

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
#ifdef FEATURE_SENSOR_GPS
	[SENSOR_TYPE_GPS]	= {
		.init		= gps_init,
		.prepare	= gps_prepare,
		.data_ready	= gps_data_ready,
		.read		= gps_read,
#ifdef FEATURE_SENSOR_GPS_ACCEL
		.txstart	= gps_txstart,
#endif
	},
#endif
#ifdef FEATURE_SENSOR_TEMP
	[SENSOR_TYPE_TEMP]	= {
		.read		= temp_read,
	},
#endif
#ifdef FEATURE_SENSOR_LIGHT
	[SENSOR_TYPE_LIGHT]	= {
		.init		= light_init,
		.read		= light_read,
	},
#endif
};

/*
 * Add sensors here if needed as:
 * sensor_type[n] = SENSOR_TYPE_XXX
 * Maximum number of sensors are defined
 * as SENSOR_MAX in sensor.h
 * */
static inline void
detect_sensor(void)
{
#if HW_IS_SOM
	sensor_type[0] = SENSOR_TYPE_TEMP;
#if HW_VERSION == HW_SOM_1_2
	sensor_type[1] = SENSOR_TYPE_GPS;
#endif
#endif
}

void
sensor_init()
{
	int	i;

	detect_sensor();
	for (i = 0; i < SENSOR_MAX; i++) {
		if (sensor_cb[sensor_type[i]].init)
			sensor_cb[sensor_type[i]].init();
	}
}

void
sensor_prepare()
{
	int	i;

	for (i = 0; i < SENSOR_MAX; i++) {
		if (sensor_cb[sensor_type[i]].prepare)
			sensor_cb[sensor_type[i]].prepare();
	}
}

ostime_t
sensor_data_ready()
{
	ostime_t	t;
	int		i;

	for (i = 0; i < SENSOR_MAX; i++) {
		if (sensor_cb[sensor_type[i]].data_ready) {
			if ((t = sensor_cb[sensor_type[i]].data_ready()) != 0)
				return t;
		}
	}
	return 0;
}

size_t
sensor_get_data(int idx, char *buf, int len)
{
	if (len <= 0 || !sensor_cb[sensor_type[idx]].read)
		return 0;
	buf[0] = sensor_type[idx];
	return 1 + sensor_cb[sensor_type[idx]].read(buf + 1, len - 1);
}

void
sensor_txstart(void)
{
	int	i;

	for (i = 0; i < SENSOR_MAX; i++) {
		if (sensor_cb[sensor_type[i]].txstart)
			sensor_cb[sensor_type[i]].txstart();
	}
}

#endif /* FEATURE_SENSOR */

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
