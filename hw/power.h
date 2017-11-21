#ifndef __POWER_H__
#define __POWER_H__

#include "hw/hw.h"

#ifdef FEATURE_POWER_SUPPLY

void	power_init(void);

#else /* !FEATURE_POWER_SUPPLY */

#define power_init()

#endif /* FEATURE_POWER_SUPPLY */

#ifdef FEATURE_POWER_SUPPLY_MULTI

#define POWER_LORA	0
#define POWER_SENSOR	1

void	power(uint8_t, bool);

#else /* !FEATURE_POWER_SUPPLY_MULTI */

#define power(what, on)

#endif /* FEATURE_POWER_SUPPLY_MULTI */

#endif /* __POWER_H__ */
