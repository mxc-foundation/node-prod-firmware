#ifndef __POWER_H__
#define __POWER_H__

#include "hw/hw.h"

#ifdef FEATURE_POWER_SUPPLY

#define POWER_LORA	0
#define POWER_SENSOR	1

void	power_init(void);
void	power(uint8_t, bool);

#else /* !FEATURE_POWER_SUPPLY */

#define power_init()
#define power(what, on)

#endif /* FEATURE_POWER_SUPPLY */

#endif /* __POWER_H__ */
