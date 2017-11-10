#ifndef __IOX_H__
#define __IOX_H__

#ifdef HW_IOX_I2C_ADDR

int	iox_conf(uint8_t pin, bool input);
int	iox_set(uint8_t pin, bool val);
int	iox_get(uint8_t pin);
int	iox_setconf(int conf);
int	iox_getconf(void);
int	iox_setpins(int pins);
int	iox_getpins(void);

#else

#define iox_conf(pin, input)	(-1)
#define iox_set(pin, val)	(-1)
#define iox_get(pin)		(-1)

#endif

#endif /* __IOX_H__ */
