#ifndef __I2C_H__
#define __I2C_H__

int	i2c_read(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len);
int	i2c_write(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len);
void	i2c_init(void);

#endif /* __I2C_H__ */
