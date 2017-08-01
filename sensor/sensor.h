#ifndef __SENSOR_H__
#define __SENSOR_H__

void		sensor_init(void);
void		sensor_prepare(void);
ostime_t	sensor_data_ready(void);
size_t		sensor_get_data(char *buf, int len);
ostime_t	sensor_period(void);
void		sensor_txstart(void);

#endif /* __SENSOR_H__ */
