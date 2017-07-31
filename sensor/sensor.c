#include <sys/types.h>
#include <FreeRTOS.h>
#include <hw_gpio.h>
#include "hw/hw.h"
#include "lmic/oslmic.h"
#include "gps.h"
#include "sensor.h"
#include "temp.h"

#define SENSOR_TYPE_UNKNOWN	0
#define SENSOR_TYPE_GPS		1
#define SENSOR_TYPE_TEMP	2
static uint8_t	sensor_type;

struct sensor_callbacks {
	void		(*init)(void);
	void		(*prepare)(void);
	ostime_t	(*data_ready)(void);
	int		(*read)(char *, int);
};

const struct sensor_callbacks	sensor_cb[] = {
	[SENSOR_TYPE_UNKNOWN]	= {
	},
	[SENSOR_TYPE_GPS]	= {
		.init		= gps_init,
		.prepare	= gps_prepare,
		.data_ready	= gps_data_ready,
		.read		= gps_read,
	},
	[SENSOR_TYPE_TEMP]	= {
		.read		= temp_read,
	},
};

static void
sensor_power_init(void)
{
	hw_gpio_set_pin_function(HW_SENSOR_PS_PORT,  HW_SENSOR_PS_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(HW_SENSOR_EN_PORT,  HW_SENSOR_EN_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_pin_function(HW_IMU_BACKUP_PORT, HW_IMU_BACKUP_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO);
	hw_gpio_set_active(HW_SENSOR_PS_PORT,  HW_SENSOR_PS_PIN);
	hw_gpio_set_active(HW_SENSOR_EN_PORT,  HW_SENSOR_EN_PIN);
	hw_gpio_set_active(HW_IMU_BACKUP_PORT, HW_IMU_BACKUP_PIN);
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
	return sec2osticks(60);
}
