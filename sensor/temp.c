#include <limits.h>
#include <ad_temp_sens.h>

#include "hw/hw.h"

#ifdef FEATURE_SENSOR_TEMP

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

#endif /* FEATURE_SENSOR_TEMP */
