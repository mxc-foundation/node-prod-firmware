#include <sys/types.h>
#include <hw_gpio.h>
#include "gps.h"
#include "hw.h"
#include "led.h"
#include "sensor.h"
#include <stdio.h>

#define SENSOR_TYPE_UNKNOWN	0
#define SENSOR_TYPE_GPS		1
static uint8_t	sensor_type = SENSOR_TYPE_UNKNOWN;

struct sensor_callbacks {
	void	(*init)(void);
	int	(*read)(char *buf, int len);
};

struct sensor_callbacks	sensor_cb[] = {
	[SENSOR_TYPE_UNKNOWN]	= { NULL, NULL },
	[SENSOR_TYPE_GPS]	= { gps_init, gps_read },
};

static void
sensor_power_init(void)
{
	hw_gpio_set_pin_function(HW_SENSOR_POWER_PORT, HW_SENSOR_POWER_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_active(HW_SENSOR_POWER_PORT, HW_SENSOR_POWER_PIN);
}

static inline void
detect_sensor(void)
{
	sensor_type = SENSOR_TYPE_GPS;
}

void
sensor_init()
{
	printf("sensor init\r\n");
	sensor_power_init();
	detect_sensor();
	if (sensor_cb[sensor_type].init)
		sensor_cb[sensor_type].init();
}

size_t
sensor_get_data(char *buf, int len)
{
	int	rlen;

	if (len <= 0)
		return 0;
	buf[0] = sensor_type;
	rlen = 1;
	switch (sensor_type) {
	case SENSOR_TYPE_GPS:
		rlen += gps_read(buf + 1, len - 1);
		led_set_status(rlen > 1 ? LED_BLINK_GREEN : LED_BLINK_RED);
		printf("GPS: ");
		for (int i = 0; i < rlen; i++)
			printf("%02x", (uint8_t)buf[i]);
		printf("\r\n");
		break;
	}
	return rlen;
}
