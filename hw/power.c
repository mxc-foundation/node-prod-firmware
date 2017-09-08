#include <FreeRTOS.h>
#include <hw_gpio.h>

#include "hw.h"
#include "lora/util.h"
#include "power.h"

static const struct {
	uint8_t	port, pin;
} ps[] = {
	[POWER_LORA]	= { HW_LORA_EN_PS_PORT, HW_LORA_EN_PS_PIN },
	[POWER_SENSOR]	= { HW_SENSOR_EN_PORT,  HW_SENSOR_EN_PIN  },
};

INITIALISED_PRIVILEGED_DATA static uint8_t	status =
	(1 << POWER_LORA) | (1 << POWER_SENSOR);

void
power_init()
{
	int	i;

	hw_gpio_configure_pin(HW_LORA_EN_LDO_PORT,   HW_LORA_EN_LDO_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true);
	hw_gpio_configure_pin(HW_SENSOR_BACKUP_PORT, HW_SENSOR_BACKUP_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true);
	hw_gpio_configure_pin(HW_IMU_BACKUP_PORT,    HW_IMU_BACKUP_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true);
	hw_gpio_configure_pin(HW_PS_EN_PORT,         HW_PS_EN_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, status);
	for (i = 0; i < (int)ARRAY_SIZE(ps); i++) {
		hw_gpio_configure_pin(ps[i].port, ps[i].pin,
		    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, status & (1 << i));
	}
}

void
power(uint8_t what, bool on)
{
	if (what >= ARRAY_SIZE(ps))
		return;
	if (on) {
		if (!status)
			hw_gpio_set_active(HW_PS_EN_PORT, HW_PS_EN_PIN);
		status |= 1 << what;
		hw_gpio_set_active(ps[what].port, ps[what].pin);
	} else {
		hw_gpio_set_inactive(ps[what].port, ps[what].pin);
		status &= ~(1 << what);
		if (!status)
			hw_gpio_set_inactive(HW_PS_EN_PORT, HW_PS_EN_PIN);
	}
}
