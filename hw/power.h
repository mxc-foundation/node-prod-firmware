#ifndef __POWER_H__
#define __POWER_H__

#define POWER_LORA	0
#define POWER_SENSOR	1

void	power_init(void);
void	power(uint8_t, bool);

#endif /* __POWER_H__ */
