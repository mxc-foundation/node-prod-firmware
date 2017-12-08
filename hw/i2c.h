#ifndef __I2C_H__
#define __I2C_H__

#ifdef FEATURE_I2C

int	i2c_read(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len);
int	i2c_write(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len);
void	i2c_init(void);

#else

#define i2c_init()

#endif

#endif /* __I2C_H__ */
