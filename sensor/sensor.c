#include <sys/types.h>
#include <FreeRTOS.h>
#include <hw_gpio.h>
#include "hw/hw.h"
#include "lmic/oslmic.h"
#include "gps.h"
#include "sensor.h"

#define SENSOR_TYPE_UNKNOWN	0
#define SENSOR_TYPE_GPS		1
static uint8_t	sensor_type;

struct sensor_callbacks {
	void		(*init)(void);
	ostime_t	(*prepare)(void);
	int		(*read)(char *buf, int len);
};

const struct sensor_callbacks	sensor_cb[] = {
	[SENSOR_TYPE_UNKNOWN]	= { NULL, NULL },
	[SENSOR_TYPE_GPS]	= { gps_init, gps_prepare, gps_read },
};

static void
sensor_power_init(void)
{
	hw_gpio_set_pin_function(HW_SENSOR_PS_PORT, HW_SENSOR_PS_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(HW_SENSOR_EN_PORT, HW_SENSOR_EN_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(HW_SM_DIO1_PORT,   HW_SM_DIO1_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_active(HW_SENSOR_PS_PORT, HW_SENSOR_PS_PIN);
	hw_gpio_set_inactive(HW_SENSOR_EN_PORT, HW_SENSOR_EN_PIN);
	hw_gpio_set_active(HW_SM_DIO1_PORT,   HW_SM_DIO1_PIN);
}

static inline void
detect_sensor(void)
{
	sensor_type = SENSOR_TYPE_GPS;
}

void
sensor_init()
{
	sensor_power_init();
	detect_sensor();
	if (sensor_cb[sensor_type].init)
		sensor_cb[sensor_type].init();
}

ostime_t
sensor_prepare()
{
	if (sensor_cb[sensor_type].prepare)
		return sensor_cb[sensor_type].prepare();
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
