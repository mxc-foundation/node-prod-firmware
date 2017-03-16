#include <sys/types.h>

#define len_needed	1

size_t
sensor_get_data(uint8_t *buf, int len)
{
	if (len < len_needed)
		return 0;
	buf[0] = 0xa5;
	return len_needed;
}
