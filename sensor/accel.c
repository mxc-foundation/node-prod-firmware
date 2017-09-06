/* MPU-9250 support */

#include <FreeRTOS.h>
#include <hw_gpio.h>
#include <hw_i2c.h>

#include "hw/hw.h"
#include "lmic/oslmic.h"
#include "gps.h"

//#define DEBUG

#ifdef DEBUG
#include <stdio.h>
#endif

static int
mpu_read_reg(uint8_t reg)
{
	size_t			status;
	HW_I2C_ABORT_SOURCE	abort_src = HW_I2C_ABORT_NONE;
	uint8_t			val;

#ifdef DEBUG
	printf("read %02x\r\n", reg);
#endif
	hw_i2c_write_byte(HW_I2C1, reg);
	status = hw_i2c_read_buffer_sync(HW_I2C1, &val, 1, &abort_src,
	    HW_I2C_F_NONE);
	if (status < 1 || abort_src != HW_I2C_ABORT_NONE) {
#ifdef DEBUG
		printf("i2c read error: %x\r\n", abort_src);
#endif
		return -1;
	}
#ifdef DEBUG
	printf("reg %02x: %02x\r\n", reg, val);
#endif
	return val;
}

static void
mpu_write_reg(uint8_t reg, uint8_t val)
{
	size_t			status;
	HW_I2C_ABORT_SOURCE	abort_src = HW_I2C_ABORT_NONE;

#ifdef DEBUG
	printf("write %02x: %02x\r\n", reg, val);
#else
	hw_cpm_delay_usec(4000);
#endif
	hw_i2c_write_byte(HW_I2C1, reg);
	status = hw_i2c_write_buffer_sync(HW_I2C1, &val, 1, &abort_src,
	    HW_I2C_F_WAIT_FOR_STOP);
	(void)status;
#ifdef DEBUG
	if (status < 1 || abort_src != HW_I2C_ABORT_NONE)
		printf("i2c write error: %x\r\n", abort_src);
#endif
}

#define MPU_ACCEL_CONFIG_2		0x1d
#define MPU_ACCEL_FCHOICE_B_SHIFT	3
#define MPU_A_DLPF_CFG_SHIFT		0

#define MPU_LP_ACCEL_ODR		0x1e
#define MPU_LPOSC_CLKSEL_MASK		0x0f
#define MPU_LPOSC_CLKSEL_0_24_HZ	0
#define MPU_LPOSC_CLKSEL_0_49_HZ	1
#define MPU_LPOSC_CLKSEL_0_98_HZ	2
#define MPU_LPOSC_CLKSEL_1_95_HZ	3
#define MPU_LPOSC_CLKSEL_3_91_HZ	4
#define MPU_LPOSC_CLKSEL_7_81_HZ	5
#define MPU_LPOSC_CLKSEL_15_63_HZ	6
#define MPU_LPOSC_CLKSEL_31_25_HZ	7
#define MPU_LPOSC_CLKSEL_62_50_HZ	8
#define MPU_LPOSC_CLKSEL_125_HZ		9
#define MPU_LPOSC_CLKSEL_250_HZ		10
#define MPU_LPOSC_CLKSEL_500_HZ		11

#define MPU_WOM_THR			0x1f

#define MPU_INT_PIN_CFG			0x37
#define MPU_ACTL			(1 << 7)
#define MPU_OPEN			(1 << 6)
#define MPU_LATCH_INT_EN		(1 << 5)
#define MPU_INT_ANYRD_2CLEAR		(1 << 4)
#define MPU_ACTL_FSYNC			(1 << 3)
#define MPU_FSYNC_INT_MODE_EN		(1 << 2)
#define MPU_BYPASS_EN			(1 << 1)

#define MPU_INT_ENABLE			0x38
#define MPU_WOM_EN			(1 << 6)
#define MPU_FIFO_OVERFLOW_EN		(1 << 4)
#define MPU_FSYNC_INT_EN		(1 << 3)
#define MPU_RAW_RDY_EN			(1 << 0)

#define MPU_INT_STATUS			0x3a
#define MPU_WOM_INT			(1 << 6)
#define MPU_FIFO_OVERFLOW_INT		(1 << 4)
#define MPU_FSYNC_INT			(1 << 3)
#define MPU_RAW_DATA_RDY_INT		(1 << 0)

#define MPU_MOT_DETECT_CTRL		0x69
#define MPU_ACCEL_INTEL_EN		(1 << 7)
#define MPU_ACCEL_INTEL_MODE		(1 << 6)

#define MPU_PWR_MGMT_1			0x6b
#define MPU_SLEEP			(1 << 6)
#define MPU_CYCLE			(1 << 5)
#define MPU_GYRO_STANDBY		(1 << 4)

#define MPU_PWR_MGMT_2			0x6c
#define MPU_DIS_XA			(1 << 5)
#define MPU_DIS_YA			(1 << 4)
#define MPU_DIS_ZA			(1 << 3)
#define MPU_DIS_XG			(1 << 2)
#define MPU_DIS_YG			(1 << 1)
#define MPU_DIS_ZG			(1 << 0)

static void
mpu_reg_init()
{
	int	val;

	hw_i2c_disable(HW_I2C1);
	hw_i2c_set_target_address(HW_I2C1, HW_SENSOR_MPU_I2C_ADDR);
	hw_i2c_enable(HW_I2C1);
	hw_i2c_reset_abort_source(HW_I2C1);

	val = mpu_read_reg(MPU_PWR_MGMT_1);
	if (val == -1)
		return;
	val &= ~(MPU_SLEEP | MPU_CYCLE | MPU_GYRO_STANDBY);
	mpu_write_reg(MPU_PWR_MGMT_1, val);
	mpu_write_reg(MPU_PWR_MGMT_2, MPU_DIS_XG | MPU_DIS_YG | MPU_DIS_ZG);
	mpu_write_reg(MPU_ACCEL_CONFIG_2, mpu_read_reg(MPU_ACCEL_CONFIG_2) |
	    (1 << MPU_ACCEL_FCHOICE_B_SHIFT) | (1 << MPU_A_DLPF_CFG_SHIFT));
	mpu_write_reg(MPU_INT_ENABLE, MPU_WOM_EN);
	mpu_write_reg(MPU_MOT_DETECT_CTRL, mpu_read_reg(MPU_MOT_DETECT_CTRL) |
	    MPU_ACCEL_INTEL_EN | MPU_ACCEL_INTEL_MODE);
	mpu_write_reg(MPU_WOM_THR, 25);
	mpu_write_reg(MPU_LP_ACCEL_ODR, MPU_LPOSC_CLKSEL_0_98_HZ);
	mpu_write_reg(MPU_PWR_MGMT_1, val | MPU_CYCLE);
	mpu_write_reg(MPU_INT_PIN_CFG, MPU_ACTL | MPU_OPEN | MPU_LATCH_INT_EN);
}

void
accel_init()
{
	const i2c_config	i2c_cfg = {
		.speed		= HW_I2C_SPEED_FAST,
		.mode		= HW_I2C_MODE_MASTER,
		.addr_mode	= HW_I2C_ADDRESSING_7B,
		.address	= HW_SENSOR_MPU_I2C_ADDR,
	};

	hw_gpio_configure_pin(HW_SENSOR_I2C_SCL_PORT, HW_SENSOR_I2C_SCL_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_I2C_SCL, true);
	hw_gpio_configure_pin(HW_SENSOR_I2C_SDA_PORT, HW_SENSOR_I2C_SDA_PIN,
	    HW_GPIO_MODE_INPUT,  HW_GPIO_FUNC_I2C_SDA, true);
	hw_i2c_init(HW_I2C1, &i2c_cfg);
	hw_gpio_set_pin_function(HW_SENSOR_MPU_INT_PORT, HW_SENSOR_MPU_INT_PIN,
	    HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_GPIO);
}

int
accel_status()
{
	int	status;
	bool	level;

	level = hw_gpio_get_pin_status(HW_SENSOR_MPU_INT_PORT,
	    HW_SENSOR_MPU_INT_PIN);
	mpu_reg_init();
	status = mpu_read_reg(MPU_INT_STATUS);
	return status == -1 ? -1 : !level;
}
