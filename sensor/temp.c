#include <stdio.h>
#include <string.h>

#include <limits.h>
#include <ad_temp_sens.h>

#include "hw/hw.h"
#include "hw/i2c.h"
#include "temp.h"

#ifdef FEATURE_SENSOR_TEMP

#ifdef FEATURE_SENSOR_TEMP_PCT2075

// Size of temperature data
#define SZ	2

int
temp_read(char *buf, int len)
{
	if (len < SZ)
		return 0;
	if (i2c_read(HW_SENSOR_TEMP_I2C_ADDR, 0, (uint8_t *)buf, SZ) == -1)
		return 0;
	return SZ;
}

#elif defined(FEATURE_SENSOR_TEMP_INTERNAL)

int
temp_read(char *buf, int len)
{
	tempsens_source	src;
	int		temp;

	if (len < 1)
		return 0;
	src = ad_tempsens_open();
	temp = ad_tempsens_read(src);
	ad_tempsens_close(src);
	if (temp < SCHAR_MIN || temp > SCHAR_MAX)
		return 0;
	buf[0] = temp;
	return 1;
}

#else
#error "Unknown FEATURE_SENSOR_TEMP_*"
#endif

#endif /* FEATURE_SENSOR_TEMP */
