#ifndef __SENSOR_H__
#define __SENSOR_H__

#include "hw/hw.h"

ostime_t	sensor_period(void);

#ifdef FEATURE_SENSOR

void		sensor_init(void);
void		sensor_prepare(void);
ostime_t	sensor_data_ready(void);
size_t		sensor_get_data(char *buf, int len);
void		sensor_txstart(void);

#else /* !FEATURE_SENSOR */

#define sensor_init()
#define sensor_prepare()
#define sensor_data_ready()		((ostime_t)0)
#define sensor_get_data(buf, len)	((size_t)0)
#define sensor_txstart()

#endif /* FEATURE_SENSOR */

#endif /* __SENSOR_H__ */
