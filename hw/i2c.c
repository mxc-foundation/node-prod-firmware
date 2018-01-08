#include <hw_gpio.h>
#include <hw_i2c.h>

#include "hw/hw.h"
#include "hw/i2c.h"

#ifdef FEATURE_I2C

static void
i2c_set_addr(uint8_t addr)
{
	hw_i2c_disable(HW_I2C1);
	hw_i2c_set_target_address(HW_I2C1, addr);
	hw_i2c_enable(HW_I2C1);
	hw_i2c_reset_abort_source(HW_I2C1);
}

int
i2c_read(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len)
{
	size_t			status;
	HW_I2C_ABORT_SOURCE	abort_src = HW_I2C_ABORT_NONE;

	i2c_set_addr(addr);
	hw_i2c_write_byte(HW_I2C1, reg);
	status = hw_i2c_read_buffer_sync(HW_I2C1, buf, len, &abort_src,
	    HW_I2C_F_NONE);
	if (status < len || abort_src != HW_I2C_ABORT_NONE)
		return -1;
	return status;
}

int
i2c_write(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len)
{
	size_t			status;
	HW_I2C_ABORT_SOURCE	abort_src = HW_I2C_ABORT_NONE;

	i2c_set_addr(addr);
	hw_i2c_write_byte(HW_I2C1, reg);
	status = hw_i2c_write_buffer_sync(HW_I2C1, buf, len, &abort_src,
	    HW_I2C_F_WAIT_FOR_STOP);
	if (status < len || abort_src != HW_I2C_ABORT_NONE)
		return -1;
	return status;
}

void
i2c_init()
{
	const i2c_config	i2c_cfg = {
		.speed		= HW_I2C_SPEED_FAST,
		.mode		= HW_I2C_MODE_MASTER,
		.addr_mode	= HW_I2C_ADDRESSING_7B,
	};

	hw_gpio_configure_pin(HW_I2C_SCL_PORT, HW_I2C_SCL_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_I2C_SCL, true);
	hw_gpio_configure_pin(HW_I2C_SDA_PORT, HW_I2C_SDA_PIN,
	    HW_GPIO_MODE_INPUT,  HW_GPIO_FUNC_I2C_SDA, true);
	hw_i2c_init(HW_I2C1, &i2c_cfg);
}

#endif /* FEATURE_I2C */
